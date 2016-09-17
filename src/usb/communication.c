#include "usb/endpoint_0.h"
#include "usb/led.h"
#include "usb/fifo.h"
#include "usb/endpoint.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>

// Interrupt handling
#define CLEAR_UEINTX(flag) UEINTX &= ~(1<<flag)

static inline void clear_setup() {
  CLEAR_UEINTX(RXSTPI);
}
static inline void clear_in() {
  CLEAR_UEINTX(TXINI);
}
static inline void clear_out() {
  CLEAR_UEINTX(RXOUTI);
}

#define FLAG_IS_SET(reg, flag) (reg & (1<<flag))
#define CLEAR_FLAG(reg, flag) reg &= ~(1<<flag)
#define SET_FLAG(reg, flag) reg |= (1<<flag)

ISR(USB_COM_vect) {
  trip_led();

  // Process USB transfers
  if (FLAG_IS_SET(UEINT, 0)) {
    endpoint_push(0);
    static struct control_transfer_t control_transfer;
    static struct usb_setup_packet_t setup_packet;

    if (FLAG_IS_SET(UEIENX, RXSTPE) && FLAG_IS_SET(UEINTX, RXSTPI)) {
      CLEAR_FLAG(UEIENX, TXINE);
      CLEAR_FLAG(UEIENX, RXOUTE);

      if (control_transfer.stage != CTRL_IDLE && control_transfer.stage != CTRL_STALL) {
        cancel_control_transfer(&control_transfer);
      }
      control_transfer.callback_handshake = 0;
      control_transfer.callback_data_out = 0;
      control_transfer.callback_cancel = 0;
      control_transfer.stage = CTRL_SETUP;

      size_t read = fifo_read(&setup_packet, sizeof(struct usb_setup_packet_t));
      if (read == sizeof(struct usb_setup_packet_t)) {
        control_transfer.req = &setup_packet;
        process_setup(&control_transfer);
      }

      if (
           (control_transfer.stage == CTRL_DATA_IN)
        || (control_transfer.stage == CTRL_HANDSHAKE_OUT)
      ) {
        SET_FLAG(UEIENX, TXINE);
      }
      else if (
           (control_transfer.stage == CTRL_DATA_OUT)
        || (control_transfer.stage == CTRL_HANDSHAKE_IN)
      ) {
        SET_FLAG(UEIENX, RXOUTE);
      }
      else {
        SET_FLAG(UECONX, STALLRQ);
      }

      clear_setup();
    }

    if (FLAG_IS_SET(UEIENX, TXINE) && FLAG_IS_SET(UEINTX, TXINI)) {
      if (control_transfer.stage == CTRL_DATA_IN) {
        // Send remaining transaction data
        uint16_t fifo_free = fifo_size() - fifo_byte_count();
        uint16_t transfer_left = control_transfer.data_in_length - control_transfer.data_in_done;
        uint16_t length = transfer_left < fifo_free ? transfer_left : fifo_free;
        uint8_t* data = (uint8_t*) control_transfer.data_in + control_transfer.data_in_done;
        control_transfer.data_in_done += fifo_write(data, length);
        clear_in();
        if (control_transfer.data_in_done == control_transfer.data_in_length) {
          free(control_transfer.data_in);
          control_transfer.data_in = 0;
          control_transfer.stage = CTRL_HANDSHAKE_IN;
          CLEAR_FLAG(UEIENX, TXINE);
        }
      }
      else if (control_transfer.stage == CTRL_HANDSHAKE_OUT) {
        // Send ZLP handshake
        clear_in();
        if (control_transfer.callback_handshake) {
          control_transfer.stage = CTRL_POST_HANDSHAKE;
        }
        else {
          control_transfer.stage = CTRL_IDLE;
          CLEAR_FLAG(UEIENX, TXINE);
        }
      }
      else if (control_transfer.stage == CTRL_POST_HANDSHAKE) {
        control_transfer.callback_handshake(&control_transfer);
        control_transfer.stage = CTRL_IDLE;
        CLEAR_FLAG(UEIENX, TXINE);
      }
      else {
        CLEAR_FLAG(UEIENX, TXINE);
      }
    }

    if (FLAG_IS_SET(UEIENX, RXOUTE) && FLAG_IS_SET(UEINTX, RXOUTI)) {
      if (control_transfer.stage == CTRL_DATA_OUT) {
        // Process incoming data
        if (control_transfer.callback_data_out) {
          control_transfer.callback_data_out(&control_transfer);
        }
        clear_out();
        if (control_transfer.stage == CTRL_HANDSHAKE_OUT) {
          SET_FLAG(UEIENX, TXINE);
          CLEAR_FLAG(UEIENX, RXOUTE);
        }
      }
      else if (control_transfer.stage == CTRL_HANDSHAKE_IN) {
        // Acknowledge ZLP handshake
        clear_out();
        // Since acknowledging the handshake doesn't require waiting for the host, perform
        // any callback immediately
        if (control_transfer.callback_handshake) {
          control_transfer.callback_handshake(&control_transfer);
        }
        control_transfer.stage = CTRL_IDLE;
        CLEAR_FLAG(UEIENX, RXOUTE);
      }
      else {
        // Ignore data
        clear_out();
        CLEAR_FLAG(UEIENX, RXOUTE);
      }
    }

    // Restore endpoint number
    endpoint_pop();
  }

}
