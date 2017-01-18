#include "remote.h"
#include "usb/std.h"
#include "usb/led.h"
#include "usb/device.h"
#include "usb/address.h"
#include "usb/configuration.h"
#include "usb/endpoint.h"
#include "usb/endpoint_0.h"
#include "usb/remote_renderer.h"
#include "frame_timer.h"

#include "kinetis/io.h"
#include "kinetis/usb_bdt.h"

#include <stdalign.h>
#include <string.h>

static inline uint16_t min(uint16_t a, uint16_t b) {
  return a < b ? a : b;
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

static inline bool needs_extra_zlp(struct control_transfer_t* transfer) {
  // When the amount of transmitted data is less then the amount of requested data,
  // a packet smaller than wMaxPacketSize has to be sent. In case the data size is an
  // exact multiple of the endpoint buffer size, queue an extra ZLP.
  bool small_transfer = transfer->data_length < transfer->req->wLength;
  bool buffer_aligned = (transfer->data_length % endpoint_get_size(0)) == 0;
  return small_transfer && buffer_aligned;
}

#define IRQ_ENABLED_AND_SET(interrupt) \
    (USB0_INTEN & USB_INTEN_ ## interrupt ## EN) && (USB0_ISTAT & USB_ISTAT_ ## interrupt)
#define requested_wakeup() \
  ((USB0_USBTRC0 & USB_USBTRC_USBRESMEN) && (USB0_USBTRC0 & USB_USBTRC_USB_RESUME_INT))

#define DATA_INTERRUPTS (USB_INTEN_TOKDNEEN | USB_INTEN_SOFTOKEN)

void usb_isr() {
  // TODO VBUS transitions?

  if (IRQ_ENABLED_AND_SET(SOFTOK)) {
    USB0_ISTAT = USB_ISTAT_SOFTOK;
    uint16_t frame_number = ((USB0_FRMNUMH << 8) | (USB0_FRMNUML)) & 0x7FF;
    new_sof_received(frame_number);
  }

  if (IRQ_ENABLED_AND_SET(USBRST)) {
    // Clear reset interrupt, suspend and token interrupt
    USB0_ISTAT = USB_ISTAT_USBRST | USB_ISTAT_SLEEP | USB_ISTAT_TOKDNE;
    // Enable suspend and token interrupt
    USB0_INTEN |= USB_INTEN_SLEEPEN | DATA_INTERRUPTS;

    // Reset all buffer toggles to 0
    reset_buffer_toggles();

    // Load default configuration
    usb_set_address(0);
    if (set_configuration_index(0)) {
      set_device_state(DEFAULT);
    }
  }

  // Wake-up, suspend
  if (IRQ_ENABLED_AND_SET(SLEEP)) {
    // Clear and disable suspend and token interrupt
    USB0_INTEN &= ~(USB_INTEN_SLEEPEN | DATA_INTERRUPTS);
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
    USB0_INTEN |= USB_INTEN_SLEEPEN | DATA_INTERRUPTS;

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
    trip_led();

    const uint8_t token_status = pop_token_status();
    const uint8_t bdt_index = token_status >> 2;
    struct buffer_descriptor_t* bdt_entry = get_bdt() + bdt_index;

    const uint8_t endpoint = token_status >> 4;
    const enum usb_pid_t token_pid = get_token_pid(bdt_entry);

    if (endpoint == 0) {
      static struct control_transfer_t control_transfer;
      static struct usb_setup_packet_t setup_packet;
      // Keep past-the-end pointer of data buffer to know when to stop queueing data
      // control_data will track the amount of queued data, so is ahead of .data_done
      static uint8_t* control_data;
      static uint8_t* control_data_end;
      static bool queue_extra_zlp;

      if (token_pid == PID_SETUP) {
        ep_rx_buffer_pop(0);

        // Since we may be using a dynamically allocated buffer that can get discarded when
        // cancelling an ongoing transfer, copy the data _before_ doing anything else
        setup_packet = *(const struct usb_setup_packet_t*) bdt_entry->buffer;

        // Cancel pending transfers
        if (control_transfer.stage != CTRL_IDLE && control_transfer.stage != CTRL_STALL) {
          cancel_control_transfer(&control_transfer);
        }

        // Cancel all pending TX buffers
        uint8_t bank = get_buffer_bank_count();
        while (bank--) {
          get_buffer_descriptor(0, BDT_DIR_TX, bank)->desc = 0;
        }
        // Dequeue all pending RX buffers
        ep_rx_buffer_dequeue_all(0);

        // Clear TXSUSPEND/TOKENBUSY bit to resume operation
        // This should be done as early as possible
        USB0_CTL &= ~USB_CTL_TXSUSPENDTOKENBUSY;

        init_control_transfer(&control_transfer, &setup_packet);
        process_setup(&control_transfer);

        // Set TX and RX toggles to 1 in any case
        // Even with data toggle sync enabled, SETUP packages will always be accepted
        set_data_toggle(0, BDT_DIR_TX, 1);
        set_data_toggle(0, BDT_DIR_RX, 1);

        const enum control_stage_t stage = control_transfer.stage;
        if (stage == CTRL_DATA_IN) {
          control_data = (uint8_t*) control_transfer.data;
          control_data_end = control_data + control_transfer.data_length;

          queue_extra_zlp = needs_extra_zlp(&control_transfer);
          // Queue as IN many packets as we have banks available
          uint8_t tx_buffers = get_buffer_bank_count();
          uint16_t remaining = control_transfer.data_length;
          while (tx_buffers-- && remaining) {
            uint16_t queued = ep_tx_buffer_push(0, control_data, remaining);
            control_data += queued;
            remaining -= queued;
          }
          // Queue OUT buffer for ZLP/SETUP
          ep_rx_buffer_push(0, NULL, 0);
        }
        else if (stage == CTRL_DATA_OUT) {
          control_data = (uint8_t*) control_transfer.data;
          control_data_end = control_data + control_transfer.data_length;

          // Queue as many buffers as possible
          uint16_t queued = ep_rx_buffer_push(0, control_data, control_transfer.data_length);
          while (control_data != control_data_end && queued) {
            control_data += queued;
            // Check if more data should be queued
            if (control_data != control_data_end) {
              queued = ep_rx_buffer_push(0, control_data, control_data_end-control_data);
            }
          };
        }
        else if (stage == CTRL_HANDSHAKE_OUT) {
          control_data = 0;
          control_data_end = 0;
          // Queue ZLP handshake
          ep_tx_buffer_push(0, NULL, 0);
          // Queue OUT buffer for next SETUP
          ep_rx_buffer_push(0, NULL, 0);
        }
        else { // All other states are invalid at this point
          endpoint_stall(0);
          cancel_control_transfer(&control_transfer);
          // Queue OUT buffer for next SETUP
          ep_rx_buffer_push(0, NULL, 0);
        }
      }

      else if (token_pid == PID_IN) {
        if (control_transfer.stage == CTRL_DATA_IN) {
          // IN data has been transmitted. Read BD to see how much was transmitted
          control_mark_data_done(&control_transfer, get_byte_count(bdt_entry));

          if (control_data != control_data_end) {
            uint16_t remaining = control_data_end - control_data;
            control_data += ep_tx_buffer_push(0, control_data, remaining);
          }
          else if (queue_extra_zlp) {
              // Queue extra ZLP to signal request length underrun
              ep_tx_buffer_push(0, NULL, 0);
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
        ep_rx_buffer_pop(0);

        if (control_transfer.stage == CTRL_DATA_OUT) {
          // Copy data to data buffer
          uint16_t left = control_transfer.data_length - control_transfer.data_done;
          uint16_t bytes_received = get_byte_count(bdt_entry);
          // These values should be the same for the last transfer, but we want to
          // avoid buffer overflows in case they aren't.
          uint16_t size = min(bytes_received, left);

          // Only copy back if we used the endpoint's buffer
          uint8_t* dest = control_data_end - left;
          if (dest != bdt_entry->buffer) {
            memcpy(dest, bdt_entry->buffer, size);
          }

          control_mark_data_done(&control_transfer, size);

          if (control_transfer.stage == CTRL_HANDSHAKE_OUT) {
            // Queue IN ZLP
            set_data_toggle(0, BDT_DIR_TX, 1);
            ep_tx_buffer_push(0, NULL, 0);
            // OUT buffer for SETUP
            set_data_toggle(0, BDT_DIR_RX, 0);
            ep_rx_buffer_push(0, NULL, 0);
          }
          else if (control_data != control_data_end) {
            // Queue more RX buffers
            // Since the queue was entirely filled up when initialising the data stage,
            // we need only supply one new RX buffer.
            // Use a direct write for better efficiency.
            control_data += ep_rx_buffer_push(0, control_data, control_data_end-control_data);
          }
        }
        else if (control_transfer.stage == CTRL_HANDSHAKE_IN) {
          // Reception of OUT DATA1 packet means the transaction is finished
          if (bdt_entry->desc & _BV(BDT_DESC_DATA01)) {
            if (control_transfer.callback_handshake) {
              control_transfer.callback_handshake(&control_transfer);
            }
            control_transfer.stage = CTRL_IDLE;
            set_data_toggle(0, BDT_DIR_RX, 0);
            ep_rx_buffer_push(0, NULL, 0);
          }
        }
        else {
          // Received unexepcted OUT frame. Discard and requeue buffer (for SETUP).
          // We could also stall, but perhaps this is just a ZLP at the end of a data stage.
          ep_rx_buffer_push(0, NULL, 0);
        }
      }
    }

    else if (endpoint == 1 && token_pid == PID_OUT) {
      ep_rx_buffer_pop(1);

      struct remote_transfer_t* transfer = remote_renderer_get_current();
      if (transfer) {
        const size_t transferred = get_byte_count(bdt_entry);
        const size_t copy_len = min(transfer->buffer_remaining, transferred);
        if (transfer->buffer_pos != bdt_entry->buffer) {
          memcpy(transfer->buffer_pos, bdt_entry->buffer, copy_len);
        }
        transfer->buffer_pos += copy_len;
        transfer->buffer_remaining -= copy_len;

        // TODO Write directly to frame buffer
        // Writing directly to frame buffer is hard:
        //   * When finalising a new transfer, a new frame buffer is not yet available
        //   * Short buffers (< EP_SIZE) are dangerous as they might overflow
        //   * When queueing buffers, care should be taken to queue the correct offset
        ep_rx_buffer_push(1, NULL, 0);

        if (transfer->buffer_remaining == 0 || transferred < endpoint_get_size(1)) {
          if (copy_len != transferred || !remote_renderer_finish()) {
            remote_renderer_stop();
            endpoint_stall(1);
          }
        }
      }
      else {
        endpoint_stall(1);
      }
    }
  }
}
