#include "remote.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#include "usb/std.h"
#include "usb/device.h"
#include "usb/configuration.h"

#include "usb/address.h"
#include "usb/led.h"
#include "usb/endpoint_fifo.h"
#include "usb/endpoint_stack.h"
#include "usb/endpoint.h"
#include "usb/endpoint_0.h"
#include "usb/remote_renderer.h"
#include "frame_timer.h"

// Heavily based on LUFA code, stripped down to the specifics of the ATmega32U4.

// PLL stuff
static void configure_pll() {
  // Set PLL frequency to 96MHz, output scaler to 2 to generate 48MHz USB clock
  PLLFRQ = _BV(PLLUSB) | _BV(PDIV3) | _BV(PDIV1);
  // Set input prescaler to 2 (16MHz source clock)
  PLLCSR = _BV(PINDIV);
}

static void enable_pll() {
  // Enable PLL
  PLLCSR |= _BV(PLLE);
  // Wait for PLL lock
  while (!(PLLCSR & _BV(PLOCK))) {}
}

static void disable_pll() {
  PLLCSR &= ~_BV(PLLE);
}

void init_remote() {
  /* Reset all configurations to their intended state while the device is detatched.
   * Then, when everything is set up, enable the end-of-reset interrupt en attach the device.
   * If the device was already connected to the host, this will immediatly result in an interrupt,
   * thus a full configuration of the device. Otherwise, the device will be fully configured once
   * the host is connected. This ensures that the end-of-reset ISR may always assume the same
   * initial state.
   */
  USBCON = _BV(FRZCLK);
  UHWCON = 0;
  UDIEN = 0;
  // Clear all interrupts
  USBINT = 0;
  UDINT = 0;
  // -- End of state reset --

  // Enable regulator
  UHWCON = _BV(UVREGE);

  // Configure PLL
  configure_pll();
  disable_pll();
  // Enable USB interface
  USBCON = _BV(FRZCLK) | _BV(OTGPADE) | _BV(USBE) | _BV(VBUSTE);
  // TODO Decide which interrupts to enable
//  UDIEN = _BV(EORSTE) | _BV(WAKEUPE);

  // Initialise USB activity LED
  init_led();
  // Prepare to enter ATTACHED state

  // Attach pull-up
  UDCON &= ~_BV(DETACH);
  set_device_state(ATTACHED);
}

bool is_remote_connected() {
  // Communication is only possible in a configured state
  return get_device_state() == CONFIGURED;
}

#define FLAG_IS_SET(reg, flag) (reg & (1<<flag))
#define CLEAR_FLAG(reg, flag) reg &= ~(1<<flag)
#define CLEAR_INT(reg, flag) reg = ~(1<<flag)
#define SET_FLAG(reg, flag) reg |= (1<<flag)

// USB low-level interrupts
#define CLEAR_USBINT(flag) CLEAR_INT(USBINT ,flag)
#define CLEAR_UDINT(flag) CLEAR_INT(UDINT, flag)

#define vbus_change() (FLAG_IS_SET(USBINT, VBUSTI) && FLAG_IS_SET(USBCON, VBUSTE))

#define DEVICE_ENABLED_AND_SET(interrupt) \
    (FLAG_IS_SET(UDIEN, interrupt ## E) && FLAG_IS_SET(UDINT, interrupt ## I))
#define device_reset() DEVICE_ENABLED_AND_SET(EORST)
#define requested_suspend() DEVICE_ENABLED_AND_SET(SUSP)
#define requested_wakeup() DEVICE_ENABLED_AND_SET(WAKEUP)

ISR(USB_GEN_vect) {
  if (DEVICE_ENABLED_AND_SET(SOF)) {
    CLEAR_UDINT(SOFI);
    uint16_t fnum = UDFNUMH;
    fnum = (fnum << 8) | UDFNUML;
    new_sof_received(fnum);
  }

  // VBUS transitions
  if (vbus_change()) {
    CLEAR_USBINT(VBUSTI);
    if (FLAG_IS_SET(USBSTA, VBUS)) {
      // TODO reset all endpoints
      enable_pll();
      CLEAR_FLAG(USBCON, FRZCLK);
      SET_FLAG(UDIEN, EORSTE);
      SET_FLAG(UDIEN, SUSPE);
      set_device_state(POWERED);
    }
    else {
      CLEAR_FLAG(UDIEN, SUSPI);
      CLEAR_FLAG(UDIEN, WAKEUPE);
      CLEAR_FLAG(UDIEN, EORSTE);
      CLEAR_FLAG(UDIEN, SOFE);
      SET_FLAG(USBCON, FRZCLK);
      disable_pll();
      set_device_state(ATTACHED);
    }
  }

  // End-of-reset
  if (device_reset()) {
    CLEAR_UDINT(EORSTI);
    // After reset, wait for activity
    CLEAR_UDINT(WAKEUPI);
    CLEAR_FLAG(UDIEN, WAKEUPE);
    SET_FLAG(UDIEN, SUSPE);
    SET_FLAG(UDIEN, SOFE);

    // Load default configuration
    usb_set_address(0);
    if (set_configuration_index(0)) {
      set_device_state(DEFAULT);
    }
  }

  // Wake-up, suspend
  if (requested_suspend()) {
    CLEAR_FLAG(UDIEN, SUSPE);
    CLEAR_UDINT(WAKEUPI);
    SET_FLAG(UDIEN, WAKEUPE);

    SET_FLAG(USBCON, FRZCLK);
    disable_pll();
    set_device_state(SUSPENDED);
  }

  if (requested_wakeup()) {
    // Wake up device
    enable_pll();
    CLEAR_FLAG(USBCON, FRZCLK);

    // Clear WAKEUP, enable SUSPEND
    CLEAR_UDINT(SUSPI);
    CLEAR_UDINT(WAKEUPI);
    CLEAR_FLAG(UDIEN, WAKEUPE);
    SET_FLAG(UDIEN, SUSPE);

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
}

void ep1_init() {
  remote_renderer_init();
}

static inline uint16_t min(uint16_t a, uint16_t b) {
  return a < b ? a : b;
}

// Communications interrupts
#define CLI(flag) CLEAR_INT(UEINTX, flag)
#define SEI(flag) SET_FLAG(UEIENX, flag)
#define CEI(flag) CLEAR_FLAG(UEIENX, flag)

#define ENDPOINT_IRQ_ENABLED_AND_SET(interrupt) \
    (FLAG_IS_SET(UEIENX, interrupt ## E) && FLAG_IS_SET(UEINTX, interrupt ## I))

// FIXME If the USB connection is removed during transfers, resources are not freed.
// This will result in these resources being held until the next control transfer occurs.

ISR(USB_COM_vect) {
  trip_led();

  // Process USB transfers
  if (FLAG_IS_SET(UEINT, 0) && endpoint_push(0)) {
    static struct control_transfer_t control_transfer;
    static struct usb_setup_packet_t setup_packet;

    if (ENDPOINT_IRQ_ENABLED_AND_SET(RXSTP)) {
      CEI(TXINE);
      CEI(RXOUTE);

      if (control_transfer.stage != CTRL_IDLE && control_transfer.stage != CTRL_STALL) {
        cancel_control_transfer(&control_transfer);
      }

      size_t read = fifo_read(&setup_packet, sizeof(struct usb_setup_packet_t));
      init_control_transfer(&control_transfer, &setup_packet);

      if (read == sizeof(struct usb_setup_packet_t)) {
        process_setup(&control_transfer);
      }

      if (
           (control_transfer.stage == CTRL_DATA_IN)
        || (control_transfer.stage == CTRL_HANDSHAKE_OUT)
      ) {
        SEI(TXINE);
      }
      else if (
           (control_transfer.stage == CTRL_DATA_OUT)
        || (control_transfer.stage == CTRL_HANDSHAKE_IN)
      ) {
        SEI(RXOUTE);
      }
      else {
        endpoint_stall(0);
      }

      CLI(RXSTPI);
    }

    if (ENDPOINT_IRQ_ENABLED_AND_SET(TXIN)) {
      if (control_transfer.stage == CTRL_DATA_IN) {
        // Send remaining transaction data
        uint16_t fifo_free = fifo_size() - fifo_byte_count();
        uint16_t transfer_left = control_transfer.data_length - control_transfer.data_done;
        uint16_t length = min(transfer_left, fifo_free);
        uint8_t* data = (uint8_t*) control_transfer.data + control_transfer.data_done;
        control_transfer.data_done += fifo_write(data, length);

        // Perform callback if any
        // This should change the transfer stage at the end of the transfer!
        if (control_transfer.callback_data) {
          control_transfer.callback_data(&control_transfer);
        }
        CLI(TXINI);

        if (control_transfer.stage == CTRL_HANDSHAKE_IN) {
          // Make sure an the last packet of a short transfers is not just the size of the
          // endpoint buffer
          bool short_transfer = control_transfer.data_length < control_transfer.req->wLength;
          bool aligned_transfer = (control_transfer.data_length % fifo_size()) == 0;
          if (!(short_transfer && aligned_transfer)) {
            CEI(TXINE);
          }
        }
      }
      else if (control_transfer.stage == CTRL_HANDSHAKE_IN) {
        // If interrupt was left enabled, send extra ZLP
        CLI(TXINI);
        CEI(TXINE);
      }
      else if (control_transfer.stage == CTRL_HANDSHAKE_OUT) {
        // Send ZLP handshake
        CLI(TXINI);
        if (control_transfer.callback_handshake) {
          control_transfer.stage = CTRL_POST_HANDSHAKE;
        }
        else {
          control_transfer.stage = CTRL_IDLE;
          CEI(TXINE);
        }
      }
      else if (control_transfer.stage == CTRL_POST_HANDSHAKE) {
        control_transfer.callback_handshake(&control_transfer);
        control_transfer.stage = CTRL_IDLE;
        CEI(TXINE);
      }
      else {
        CEI(TXINE);
      }
    }

    if (ENDPOINT_IRQ_ENABLED_AND_SET(RXOUT)) {
      if (control_transfer.stage == CTRL_DATA_OUT) {
        // Copy incoming data
        uint16_t left = control_transfer.data_length - control_transfer.data_done;
        uint16_t size = min(left, fifo_size());
        uint8_t* data = (uint8_t*) control_transfer.data + control_transfer.data_done;
        control_transfer.data_done += fifo_read(data, size);

        // Perform callback if any
        if (control_transfer.callback_data) {
          control_transfer.callback_data(&control_transfer);
        }
        CLI(RXOUTI);
        if (control_transfer.stage == CTRL_HANDSHAKE_OUT) {
          SEI(TXINE);
          CEI(RXOUTE);
        }
      }
      else if (control_transfer.stage == CTRL_HANDSHAKE_IN) {
        // Acknowledge ZLP handshake
        CLI(RXOUTI);
        // Since acknowledging the handshake doesn't require waiting for the host, perform
        // any callback immediately
        if (control_transfer.callback_handshake) {
          control_transfer.callback_handshake(&control_transfer);
        }
        control_transfer.stage = CTRL_IDLE;
        CEI(RXOUTE);
      }
      else {
        // Ignore data
        CLI(RXOUTI);
        CEI(RXOUTE);
      }
    }

    // Restore endpoint number
    endpoint_pop();
  }

  if (FLAG_IS_SET(UEINT, 1) && endpoint_push(1)) {
    if (ENDPOINT_IRQ_ENABLED_AND_SET(RXOUT) && FLAG_IS_SET(UEINTX, RWAL)) {
      CLI(RXOUTI);

      // ~~~
      // Endpoint stack manipulations seem to be not allowed before the FIFOCON flag is cleared.
      // Only use fifo_*() functions to access the required information directly.
      // ~~~

      // If a transfer is possible, copy the received data. Otherwise, halt the endpoint.
      struct frame_transfer_state_t* transfer = remote_renderer_get_transfer_state();
      if (transfer->write_pos) {
        // Make sure we don't overrun the buffer
        const uint16_t max_len = min(transfer->buffer_end-transfer->write_pos, fifo_byte_count());
        const uint16_t transferred = fifo_read(transfer->write_pos, max_len);
        transfer->write_pos += transferred;

        const uint16_t fifo_remaining = fifo_byte_count();
        const uint16_t transfer_remaining = transfer->buffer_end - transfer->write_pos;

        // Halt endpoint on buffer overflow or buffer underflow
        if (fifo_remaining || (transferred < fifo_size() && transfer_remaining)) {
          remote_renderer_halt();
        }
        else if (transfer_remaining == 0) {
          remote_renderer_transfer_done();
        }
      }
      else {
        remote_renderer_halt();
      }

      CLEAR_FLAG(UEINTX, FIFOCON);
    }

    endpoint_pop();
  }

}
