#include "remote.h"
#include "usb/std.h"
#include "usb/led.h"
#include "usb/device.h"
#include "usb/address.h"
#include "usb/configuration.h"
#include "usb/endpoint_kinetis.h"
#include "usb/endpoint_0.h"

#include "kinetis/io.h"
#include "kinetis/usb.h"

#include <stdalign.h>
#include <string.h>

static inline uint16_t min(uint16_t a, uint16_t b) {
  return a < b ? a : b;
}

// Currently only EP0
#define MAX_ENDPOINTS 1
#define EP0_SIZE 64

// BDT needs to be aligned to a 512B boundary (i.e. 9 lower bits of address are 0)
static alignas(512) struct bdt_endpoint_t endpoint_table[MAX_ENDPOINTS];
static struct buffer_descriptor_t* const buffer_descriptor_table =
    (struct buffer_descriptor_t*) &endpoint_table[0];

// Align buffers to word boundary, just to make sure nothing weird happens with the DMA transfers
static alignas(4) uint8_t ep0_rx_buffer[2][EP0_SIZE];

// BDT entries are 8 bytes in size, so shift back address bits by 3 positions
static inline ptrdiff_t bdt_index(uint8_t epnum, uint8_t tx, uint8_t odd) {
  return (epnum << 2) | (tx << 1) | odd;
}

static inline uint8_t get_token_pid(const struct buffer_descriptor_t* descriptor) {
  return (descriptor->desc >> 2) & 0xF;
}

static inline uint16_t byte_count(const struct buffer_descriptor_t* descriptor) {
  return (descriptor->desc >> 16) & 0x3FF;
}

static inline uint32_t generate_buffer_descriptor(uint16_t length, uint8_t data_toggle) {
  const uint32_t base_desc = _BV(BDT_DESC_OWN) | _BV(BDT_DESC_DTS);
  return base_desc | ((length & 0x3FF) << BDT_DESC_BC) | (data_toggle << BDT_DESC_DATA01);
}

static uint8_t ep0_tx_data_toggle;
static uint8_t ep0_tx_buffer_toggle;

static struct buffer_descriptor_t* get_tx_bd(uint8_t endpoint) {
  const uint16_t index = bdt_index(endpoint, BDT_DIR_TX, ep0_tx_buffer_toggle);
  if (!(buffer_descriptor_table[index].desc & BDT_DESC_OWN)) {
    ep0_tx_buffer_toggle ^= 1;
    return &buffer_descriptor_table[index];
  }
  else {
    return 0;
  }
}

static void init_ep0_bdt() {
  ep0_tx_buffer_toggle = 0;
  for (unsigned odd = 0; odd < 2; ++odd) {
    buffer_descriptor_table[bdt_index(0, BDT_DIR_TX, odd)] = (struct buffer_descriptor_t) {0, 0};
    buffer_descriptor_table[bdt_index(0, BDT_DIR_RX, odd)] =
        (struct buffer_descriptor_t) {generate_buffer_descriptor(EP0_SIZE, 0), &ep0_rx_buffer[odd]};
  }
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
  memset(&endpoint_table, 0, sizeof(endpoint_table));

  USB0_BDTPAGE1 = ((ptrdiff_t) (&buffer_descriptor_table[0]) >> 8) & 0xFE;
  USB0_BDTPAGE2 = ((ptrdiff_t) (&buffer_descriptor_table[0]) >> 16) & 0xFF;
  USB0_BDTPAGE3 = (ptrdiff_t) (&buffer_descriptor_table[0]) >> 24;

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
  USB0_ERREN = 0;

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

static uint16_t queue_in_data(void* buffer, const uint16_t max_length) {
  struct buffer_descriptor_t* bd = get_tx_bd(0);
  uint16_t remaining = max_length;

  while (bd && remaining) {
    // Send remaining transaction data
    uint16_t packet_size = min(remaining, EP0_SIZE);
    bd->desc = generate_buffer_descriptor(packet_size, ep0_tx_data_toggle);
    bd->buffer = buffer;

    buffer = (uint8_t*) buffer + packet_size;
    remaining -= packet_size;

    // Toggle DATA0/1 field
    ep0_tx_data_toggle ^= 1;
    bd = get_tx_bd(0);
  }

  return max_length-remaining;
}

static bool queue_in_zlp() {
  struct buffer_descriptor_t* bd = get_tx_bd(0);

  if (bd) {
    bd->desc = generate_buffer_descriptor(0, 1);
    bd->buffer = 0;
    ep0_tx_data_toggle = 0;
  }

  return bd != 0;
}

static void return_ep0_rx(struct buffer_descriptor_t* bd) {
  bd->desc = generate_buffer_descriptor(EP0_SIZE, 0);
}


#define IRQ_ENABLED_AND_SET(interrupt) \
    (USB0_INTEN & USB_INTEN_ ## interrupt ## EN) && (USB0_ISTAT & USB_ISTAT_ ## interrupt)
#define requested_wakeup() \
  ((USB0_USBTRC0 & USB_USBTRC_USBRESMEN) && (USB0_USBTRC0 & USB_USBTRC_USB_RESUME_INT))

void usb_isr() {
  // TODO VBUS transitions?

  if (IRQ_ENABLED_AND_SET(USBRST)) {
    // Clear reset interrupt, suspend and token interrupt
    USB0_ISTAT = USB_ISTAT_USBRST | USB_ISTAT_SLEEP | USB_ISTAT_TOKDNE;
    // Enable suspend and token interrupt
    USB0_INTEN |= USB_INTEN_SLEEPEN | USB_INTEN_TOKDNEEN;

    // Load default configuration
    usb_set_address(0);
    if (set_configuration_index(0)) {
      // After configuration, reset all buffer toggles to 0, and initialise EP0 BDT entries
      USB0_CTL |= USB_CTL_ODDRST;
      init_ep0_bdt();

      set_device_state(DEFAULT);
    }
  }

  // Wake-up, suspend
  if (IRQ_ENABLED_AND_SET(SLEEP)) {
    // Clear and disable suspend and token interrupt
    USB0_INTEN &= ~(USB_INTEN_SLEEPEN | USB_INTEN_TOKDNEEN);
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

  // TODO STALL interrupts: catch stall interrupts to clear EP0 stalls?

  if (IRQ_ENABLED_AND_SET(TOKDNE)) {
    trip_led();

    const uint8_t token_status = pop_token_status();
    const uint8_t bdt_index = token_status >> 2;
    const uint8_t endpoint = token_status >> 4;
    struct buffer_descriptor_t* bdt_entry = &buffer_descriptor_table[bdt_index];

    if (endpoint == 0) {
      static struct control_transfer_t control_transfer;
      static struct usb_setup_packet_t setup_packet;
      // Keep past-the-end pointer of data buffer to know when to stop queueing data
      // The .data pointer will track the amount of queued data, so is ahead of .data_done
      static uint8_t* control_data_end;

      const enum usb_pid_t token_pid = get_token_pid(bdt_entry);

      if (token_pid == PID_SETUP) {
        // Cancel pending transfers
        if (control_transfer.stage != CTRL_IDLE && control_transfer.stage != CTRL_STALL) {
          cancel_control_transfer(&control_transfer);
        }

        memcpy(&setup_packet, bdt_entry->buffer, sizeof(struct usb_setup_packet_t));
        return_ep0_rx(bdt_entry);

        init_control_transfer(&control_transfer, &setup_packet);
        process_setup(&control_transfer);

        // Always start with DATA1 after SETUP
        ep0_tx_data_toggle = 1;

        if (control_transfer.stage == CTRL_DATA_IN) {
          control_data_end = (uint8_t*) control_transfer.data + control_transfer.data_length;
          uint16_t queued = queue_in_data(control_transfer.data, control_transfer.data_length);
          control_transfer.data = (uint8_t*) control_transfer.data + queued;
        }
        else if (control_transfer.stage == CTRL_HANDSHAKE_OUT) {
          queue_in_zlp();
        }
        else if (
                 control_transfer.stage == CTRL_DATA_OUT
              || control_transfer.stage == CTRL_HANDSHAKE_IN
        ) {
          // Wait for OUT packet
        }
        else {
          // TODO Stall endpoint (via BDT?)
          //endpoint_stall(0);
        }

        // Clear TXSUSPEND/TOKENBUSY bit to resume operation
        USB0_CTL &= ~USB_CTL_TXSUSPENDTOKENBUSY;
      }
      else if (token_pid == PID_IN) {
        if (control_transfer.stage == CTRL_DATA_IN) {
          // IN data has been transmitted. Read BD to see how much was transmitted
          control_transfer.data_done += byte_count(bdt_entry);

          if (control_transfer.callback_data) {
            control_transfer.callback_data(&control_transfer);
          }

          if (control_transfer.stage == CTRL_HANDSHAKE_IN) {
            // Restore data buffer pointer and wait for OUT ZLP
            control_transfer.data =
                  (uint8_t*) control_transfer.data - control_transfer.data_length;
          }
          else if (control_transfer.data != (void*) control_data_end) {
            uint16_t remaining = (uint8_t*) control_transfer.data - control_data_end;
            uint16_t queued = queue_in_data(control_transfer.data, remaining);
            control_transfer.data = (uint8_t*) control_transfer.data + queued;
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
          uint16_t bytes_received = byte_count(bdt_entry);
          // These values should be the same for the last transfer, but we want to
          // avoid buffer overflows in case they aren't.
          uint16_t size = min(bytes_received, left);

          // Only copy back if we used the local buffer
          uint16_t dest_remaining = control_transfer.data_length - control_transfer.data_done;
          uint8_t* dest = (uint8_t*) control_data_end - dest_remaining;
          memcpy(dest, bdt_entry->buffer, size);

          // Keep EP0 FSM informed
          control_transfer.data_done += size;
          if (control_transfer.callback_data) {
            control_transfer.callback_data(&control_transfer);
          }

          if (control_transfer.stage == CTRL_HANDSHAKE_OUT) {
            // Restore data pointer, queue IN ZLP handshake
            control_transfer.data =
                  (uint8_t*) control_transfer.data - control_transfer.data_length;
            queue_in_zlp();
          }
          else if (control_transfer.data != (void*) control_data_end) {
            // Queue more RX buffers
            uint16_t queue_left = control_data_end - (uint8_t*) control_transfer.data;
            uint16_t rx_queued = min(queue_left, EP0_SIZE);
            control_transfer.data = (uint8_t*) control_transfer.data + rx_queued;
          }
        }
        else if (control_transfer.stage == CTRL_HANDSHAKE_IN) {
          // Reception of OUT packet means the transaction is finished
          if (control_transfer.callback_handshake) {
            control_transfer.callback_handshake(&control_transfer);
          }
          control_transfer.stage = CTRL_IDLE;
        }

        // Hand BDT entry back
        return_ep0_rx(bdt_entry);
      }
    }
  }
}

