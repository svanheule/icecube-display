#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#include "serial.h"
#include "frame_buffer.h"
#include "demo.h"

#define BAUD_RATE 115200UL

// Display global FSM
static volatile enum usart_state_t usart_state = USART_LOCAL_MODE;
static struct frame_buffer_t* frame;
static volatile uint8_t* write_ptr;
static volatile uint8_t* frame_end;

void init_serial_port() {
  // The minimal throughput for 25 FPS is 58500 baud.
  // This implies that 115200 baud is the minimal usable transmission rate.
  // Set baud rate to 115.2k, using 16MHz system clock
  const uint16_t baud_rate_register = ((F_CPU + 4*BAUD_RATE)/(8*BAUD_RATE) - 1);
  UBRR0H = (uint8_t) ((baud_rate_register >> 8) & 0x0F);
  UBRR0L = (uint8_t) baud_rate_register;

  // Use double speed, just like the original Arduino firmware
  UCSR0A = _BV(U2X0);
  // Enable Rx, Tx, and Rx interrupts
  UCSR0B = _BV(RXCIE0) | _BV(RXEN0) | _BV(TXEN0);
  // Set mode to async, 8 bit, no parity, 1 stop bit
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

  // Initialize into local mode
  usart_state = USART_LOCAL_MODE;
}

// Forward declaration
static void advance_usart_state(uint8_t word);
enum usart_state_t get_usart_state() {
  return usart_state;
}

// Transmission of char buffer (string)
struct transmit_buffer_t {
  const char* string;
  size_t pos;
  size_t length;
};
static volatile struct transmit_buffer_t transmit_buffer;

ISR(USART_UDRE_vect) {
  // Send next char
  UDR0 = transmit_buffer.string[transmit_buffer.pos++];
  // Disable interrupts if last char is sent
  if (transmit_buffer.pos == transmit_buffer.length) {
    UCSR0B &= ~(1<<UDRIE0);
  }
}


static void transmit_string(const char* string, const size_t length) {
  // Enable buffer empty IRQ
  if (length) {
    transmit_buffer = (struct transmit_buffer_t) {string, 0, length};
    cli();
    UCSR0B |= (1<<UDRIE0);
    sei();
  }
}


ISR(USART_RX_vect) {
  // Check status before reading word
  //const unsigned char status = UCSR0A;
  //if (status & (1<<FE0)) Frame error
  //if (status & (1<<DOR0)) Data overrun (FIFO full)
  //if (status & (1<<UPE0)) Parity error

  uint8_t word = UDR0;

  // Advance state machine
  advance_usart_state(word);
}

static void advance_usart_state(uint8_t word) {
  switch (usart_state) {
    case USART_WAIT:
      switch (word) {
        case COMMAND_FRAME:
          frame = create_frame();
          frame->flags = FRAME_FREE_AFTER_DRAW;
          write_ptr = (uint8_t*) frame->buffer;
          frame_end = write_ptr + sizeof(frame_t);
          usart_state = USART_FRAME;
          break;
        case COMMAND_DEMO:
          usart_state = USART_DEMO;
          break;
        case COMMAND_TEST_RING:
          usart_state = USART_TEST_RING;
          break;
        case COMMAND_TEST_SNAKE:
          usart_state = USART_TEST_SNAKE;
          break;
        case COMMAND_GET_ID:
          transmit_string("IT78:1:0", 8);
          break;
        case COMMAND_LOCAL_MODE:
          usart_state = USART_LOCAL_MODE;
          break;
        default:
          break;
        }
      break;

    case USART_FRAME:
      *write_ptr = word;
      if (++write_ptr == frame_end) {
        // Frame transfer is completed, push to queue or drop if there is no room
        if (!frame_queue_full()) {
          push_frame(frame);
        }
        else {
          free(frame);
        }
        frame = 0;
        usart_state = USART_WAIT;
      }
      break;

    case USART_DEMO:
      if (word == COMMAND_DEMO) {
        usart_state = USART_WAIT;
      }
      break;
    case USART_TEST_RING:
      if (word == COMMAND_TEST_RING) {
        usart_state = USART_WAIT;
      }
      break;

    case USART_TEST_SNAKE:
      if (word == COMMAND_TEST_SNAKE) {
        usart_state = USART_WAIT;
      }
      break;

    case USART_LOCAL_MODE:
      if (word == COMMAND_LOCAL_MODE) {
        usart_state = USART_WAIT;
      }
      break;

    // In case the state machine is in an undefined state, return to USART_WAIT.
    // This should not happen, but you never know...
    default:
      usart_state = USART_WAIT;
      break;
  }
}


uint8_t is_remote_connected() {
  // Only return TRUE when the remote connection has been set to local mode.
  return usart_state != USART_LOCAL_MODE;
}
