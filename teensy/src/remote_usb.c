#include "remote.h"
#include "usb/std.h"
#include "usb/led.h"
#include "usb/device.h"
#include "usb/address.h"
#include "usb/configuration.h"
#include "usb/endpoint.h"
#include "usb/endpoint_0.h"

#include "kinetis/io.h"
#include "kinetis/usb_bdt.h"

#include <stdalign.h>
#include <string.h>

static inline uint16_t min(uint16_t a, uint16_t b) {
  return a < b ? a : b;
}

// Currently only EP0
#define EP0_SIZE 64

// Align buffers to word boundary, just to make sure nothing weird happens with the DMA transfers
static alignas(4) uint8_t ep0_rx_buffer[EP0_SIZE];

static void init_ep0_bdt() {
  get_buffer_descriptor(0, BDT_DIR_TX, 0)->desc = 0;

  struct buffer_descriptor_t* rx = get_buffer_descriptor(0, BDT_DIR_RX, 0);
  rx->desc = generate_bdt_descriptor(EP0_SIZE, 0);
  rx->buffer = &ep0_rx_buffer;
}

void init_remote() {
  // Based on PJCR code (cores/teensy3/usb_dev.c)
  SIM_SOPT2 |= SIM_SOPT2_USBSRC | SIM_SOPT2_PLLFLLSEL;
  SIM_CLKDIV2 = SIM_CLKDIV2_USBDIV(1);

  ATOMIC_REGISTER_BIT_SET(SIM_SCGC4, 18); // Enable USBOTG clock
  NVIC_DISABLE_IRQ(IRQ_USBOTG);

  // Reset USB module and wait two USB clock cycles (see K20 reference manual)
  USB0_USBTRC0 |= USB_USBTRC_USBRESET;
  while (USB0_USBTRC0 & USB_USBTRC_USBRESET) {}

  // Reset all endpoints
  clear_bdt();

  struct buffer_descriptor_t* bdt = get_bdt();
  USB0_BDTPAGE1 = ((intptr_t) bdt >> 8) & 0xFE;
  USB0_BDTPAGE2 = ((intptr_t) bdt >> 16) & 0xFF;
  USB0_BDTPAGE3 = (intptr_t) bdt >> 24;

  // Clear pending interrupts we care about
  USB0_ISTAT = 0xFF;

  init_led();

  // Enable USB module
  USB0_CTL = USB_CTL_USBENSOFEN;
  USB0_USBCTRL = 0;

  // Attach device: enable D+ pull-up for full-speed mode
  USB0_CONTROL = USB_CONTROL_DPPULLUPNONOTG;
  set_device_state(ATTACHED);

  // Enable interrupts, no OTG interrupts, no error interrupts
  USB0_INTEN = USB_INTEN_SLEEPEN | USB_INTEN_USBRSTEN;
  USB0_OTGICR = 0;

  NVIC_ENABLE_IRQ(IRQ_USBOTG);
}

bool is_remote_connected() {
  return get_device_state() == CONFIGURED;
}

static inline uint8_t pop_token_status() {
  // Read token status
  const uint8_t status = USB0_STAT;
  // Clear token interrupt *after* reading status
  USB0_ISTAT = USB_ISTAT_TOKDNE;
  return status;
}

// TODO When receiving a display frame, use the BDT entries to write the RXOUT data
//      directly to the frame buffer

static uint16_t queue_in(const void* data, uint16_t max_length, uint8_t data01) {
  struct buffer_descriptor_t* bd = get_buffer_descriptor(0, BDT_DIR_TX, 0);
  bool buffer_available = !(bd->desc & _BV(BDT_DESC_OWN));

  if (buffer_available) {
    uint16_t packet_size = min(max_length, EP0_SIZE);
    // Send remaining transaction data
    bd->desc = generate_bdt_descriptor(packet_size, data01);
    bd->buffer = (void*) data;
    set_data_toggle(0, 1, data01^1);
    return packet_size;
  }
  else {
    return 0;
  }
}

static void return_ep0_rx(
      struct buffer_descriptor_t* bd
    , void* buffer
    , uint16_t length
    , uint8_t data01
) {
  bd->buffer = buffer;
  bd->desc = generate_bdt_descriptor(length, data01);
  set_data_toggle(0, 0, data01);
}

#define IRQ_ENABLED_AND_SET(interrupt) \
    (USB0_INTEN & USB_INTEN_ ## interrupt ## EN) && (USB0_ISTAT & USB_ISTAT_ ## interrupt)
#define requested_wakeup() \
  ((USB0_USBTRC0 & USB_USBTRC_USBRESMEN) && (USB0_USBTRC0 & USB_USBTRC_USB_RESUME_INT))

void usb_isr() {
  // TODO VBUS transitions?
  static bool frame_num_rolled_over;
  static uint16_t frame_num;
  static uint16_t pps;

  if (IRQ_ENABLED_AND_SET(SOFTOK)) {
    USB0_ISTAT = USB_ISTAT_SOFTOK;
    const uint16_t MAX_FRMNUM = 1<<11;
    uint16_t new_frame_num = ((USB0_FRMNUMH << 8) | USB0_FRMNUML) & (MAX_FRMNUM - 1);
    // Unwrap if roll-over has occured
    if (new_frame_num < frame_num) {
      frame_num_rolled_over = true;
      pps += new_frame_num + MAX_FRMNUM - frame_num;
    }
    else {
      pps += new_frame_num - frame_num;
    }

    if (!frame_num_rolled_over) {
      pps = 0;
    }
    else if (pps >= 1000) {
      trip_led();
      pps = pps % 1000;
    }
    frame_num = new_frame_num;
  }

  if (IRQ_ENABLED_AND_SET(USBRST)) {
    // Clear reset interrupt, suspend and token interrupt
    USB0_ISTAT = USB_ISTAT_USBRST | USB_ISTAT_SLEEP | USB_ISTAT_TOKDNE;
    // Enable suspend and token interrupt
    USB0_INTEN |= USB_INTEN_SLEEPEN | USB_INTEN_TOKDNEEN | USB_INTEN_SOFTOKEN;

    // Reset all buffer toggles to 0
    USB0_CTL |= USB_CTL_ODDRST;
    init_ep0_bdt();

    frame_num_rolled_over = false;
    frame_num = 0;
    pps = 0;

    // Load default configuration
    usb_set_address(0);
    if (set_configuration_index(0)) {
      set_device_state(DEFAULT);
    }
  }

  // Wake-up, suspend
  if (IRQ_ENABLED_AND_SET(SLEEP)) {
    // Clear and disable suspend and token interrupt
    USB0_INTEN &= ~(USB_INTEN_SLEEPEN | USB_INTEN_TOKDNEEN | USB_INTEN_SOFTOKEN);
    USB0_ISTAT = USB_ISTAT_SLEEP | USB_ISTAT_TOKDNE;
    // Suspend transceiver
    USB0_USBCTRL |= USB_USBCTRL_SUSP;
    // Enable wakeup interrupt
    USB0_USBTRC0 |= USB_USBTRC_USBRESMEN;

    set_device_state(SUSPENDED);
  }

  if (requested_wakeup()) {
    // Disable wakeup interrupt
    // Clear interrupt?
    // K20 reference manual doesn't mention anything about this...
    USB0_USBTRC0 &= ~(USB_USBTRC_USBRESMEN | USB_USBTRC_USB_RESUME_INT);
    // Take transceiver out of suspend state
    USB0_USBCTRL &= ~USB_USBCTRL_SUSP;
    // Enable suspend and token interrupt
    USB0_ISTAT = USB_ISTAT_SLEEP;
    USB0_INTEN |= USB_INTEN_SLEEPEN | USB_INTEN_TOKDNEEN;

    // Track device state
    if (get_configuration_index() > 0) {
      set_device_state(CONFIGURED);
    }
    else if (usb_get_address() != 0) {
      set_device_state(ADDRESSED);
    }
    else if (get_configuration_index() == 0) {
      set_device_state(DEFAULT);
    }
    else {
      set_device_state(POWERED);
    }
  }

  if (IRQ_ENABLED_AND_SET(TOKDNE)) {

    const uint8_t token_status = pop_token_status();
    const uint8_t bdt_index = token_status >> 2;
    const uint8_t endpoint = token_status >> 4;
    struct buffer_descriptor_t* bdt_entry = get_bdt() + bdt_index;

    if (endpoint == 0) {
      static struct control_transfer_t control_transfer;
      static struct usb_setup_packet_t setup_packet;
      // Keep past-the-end pointer of data buffer to know when to stop queueing data
      // control_data will track the amount of queued data, so is ahead of .data_done
      static uint8_t* control_data;
      static uint8_t* control_data_end;

      const enum usb_pid_t token_pid = get_token_pid(bdt_entry);

      if (token_pid == PID_SETUP) {
        // Since we may be using a dynamically allocated buffer that can get discarded when
        // cancelling an ongoing transfer, copy the data _before_ doing anything else
        setup_packet = *(const struct usb_setup_packet_t*) bdt_entry->buffer;

        // Cancel pending transfers
        if (control_transfer.stage != CTRL_IDLE && control_transfer.stage != CTRL_STALL) {
          cancel_control_transfer(&control_transfer);
        }
        get_buffer_descriptor(0, BDT_DIR_TX, 0)->desc = 0;

        // Wait for DATA1 OUT transfer.
        // This will be either first OUT data packet or the IN status stage (handshake)
        void* rx_buffer = &ep0_rx_buffer[0];
        uint16_t rx_len = EP0_SIZE;

        init_control_transfer(&control_transfer, &setup_packet);
        process_setup(&control_transfer);

        // Clear TXSUSPEND/TOKENBUSY bit to resume operation
        USB0_CTL &= ~USB_CTL_TXSUSPENDTOKENBUSY;

        const enum control_stage_t stage = control_transfer.stage;
        if (stage == CTRL_DATA_IN || stage == CTRL_DATA_OUT) {
          control_data = (uint8_t*) control_transfer.data;
          control_data_end = control_data + control_transfer.data_length;
          if (stage == CTRL_DATA_IN) {
            // Queue at most two packets
            control_data += queue_in(control_data, control_transfer.data_length, 1);
          }
          else {
            rx_len = min(rx_len, control_transfer.data_length);
            if (rx_len >= EP0_SIZE) {
              rx_buffer = control_data;
            }
            control_data += rx_len;
          }
        }
        else if (stage == CTRL_HANDSHAKE_OUT) {
          control_data = 0;
          control_data_end = 0;
          // Queue ZLP handshake
          queue_in(0, 0, 1);
        }
        else { // All other states are invalid at this point
          endpoint_stall(0);
          cancel_control_transfer(&control_transfer);
        }

        return_ep0_rx(bdt_entry, rx_buffer, rx_len, 1);
      }
      else if (token_pid == PID_IN) {
        if (control_transfer.stage == CTRL_DATA_IN) {
          static bool queue_extra_zlp;
          // IN data has been transmitted. Read BD to see how much was transmitted
          control_mark_data_done(&control_transfer, get_byte_count(bdt_entry));

          if (control_data != control_data_end) {
            uint16_t remaining = control_data_end - control_data;
            control_data += queue_in(control_data, remaining, get_data_toggle(0, 1));
            if (control_data_end == control_data) {
              // When the amount of transmitted data is less then the amount of requested data,
              // a packet smaller than wMaxPacketSize has to be sent. In case the data size is an
              // exact multiple of the endpoint buffer size, queue an extra ZLP.
              bool small_transfer = control_transfer.data_length < control_transfer.req->wLength;
              bool buffer_aligned = (control_transfer.data_length % EP0_SIZE) == 0;
              queue_extra_zlp = small_transfer && buffer_aligned;
            }
            else {
              queue_extra_zlp = false;
            }
          }
          else if (queue_extra_zlp) {
              // Queue extra ZLP to signal request length underrun
              queue_in(0, 0, get_data_toggle(0, 1));
              queue_extra_zlp = false;
          }
        }
        else if (control_transfer.stage == CTRL_HANDSHAKE_OUT) {
          // IN ZLP packet is transmitted, so wrap up transfer
          if (control_transfer.callback_handshake) {
            control_transfer.callback_handshake(&control_transfer);
          }
          control_transfer.stage = CTRL_IDLE;
        }
      }
      else if (token_pid == PID_OUT) {
        if (control_transfer.stage == CTRL_DATA_OUT) {
          // Copy data to data buffer
          uint16_t left = control_transfer.data_length - control_transfer.data_done;
          uint16_t bytes_received = get_byte_count(bdt_entry);
          // These values should be the same for the last transfer, but we want to
          // avoid buffer overflows in case they aren't.
          uint16_t size = min(bytes_received, left);

          // Only copy back if we used the local buffer
          uint8_t* dest = control_data_end - left;
          if (dest != bdt_entry->buffer) {
            memcpy(dest, bdt_entry->buffer, size);
          }

          control_mark_data_done(&control_transfer, size);

          if (control_transfer.stage == CTRL_HANDSHAKE_OUT) {
            queue_in(0, 0, 1);
            return_ep0_rx(bdt_entry, ep0_rx_buffer, EP0_SIZE, 0);
          }
          else if (control_data != control_data_end) {
            // Queue more RX buffers
            uint16_t queue_left = control_data_end - control_data;
            uint16_t queued = min(queue_left, EP0_SIZE);
            void* rx_buffer;
            if (queued < EP0_SIZE) {
              rx_buffer = ep0_rx_buffer;
            }
            else {
              rx_buffer = control_data;
            }

            return_ep0_rx(bdt_entry, rx_buffer, queued, get_data_toggle(0, 0) ^ 1);
            control_data += queued;
          }
        }
        else if (control_transfer.stage == CTRL_HANDSHAKE_IN) {
          // Reception of OUT DATA1 packet means the transaction is finished
          if (bdt_entry->desc & _BV(BDT_DESC_DATA01)) {
            if (control_transfer.callback_handshake) {
              control_transfer.callback_handshake(&control_transfer);
            }
            control_transfer.stage = CTRL_IDLE;
            return_ep0_rx(bdt_entry, ep0_rx_buffer, EP0_SIZE, 0);
          }
          else {
            return_ep0_rx(bdt_entry, ep0_rx_buffer, EP0_SIZE, 1);
          }
        }
        else {
          return_ep0_rx(bdt_entry, ep0_rx_buffer, EP0_SIZE, 1);
        }
      }
    }
  }
}

