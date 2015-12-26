#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "serial.h"
#include "frame_buffer.h"

void init_serial_port() {
  // The minimal throughput for 25 FPS is 58500 baud.
  // This implies that 115200 baud is the minimal usable transmission rate.
  // Set baud rate to 115.2k, using 16MHz system clock
  UCSR0A = (0<<U2X0);
  const uint16_t baud_rate_register = 7; // floor(16000000/(16*115200)-1);
  UBRR0H = (uint8_t) ((baud_rate_register >> 8) & 0x0F);
  UBRR0L = (uint8_t) baud_rate_register;
  // Enable Rx, Tx, and Rx interrupts
  // Enable USART RX interrupts
  UCSR0B = (1<<RXCIE0) | (1<<RXEN0) | (1<<TXEN0);
  // Set mode to async, 8 bit, no parity, 1 stop bit
  UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
}

// Display global FSM
static volatile enum usart_state_t usart_state = USART_WAIT;
static volatile uint8_t* write_ptr;
static volatile uint8_t* frame_end;

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
    UCSR0B |= (1<<UDRIE0);
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
  switch (usart_state) {
    case USART_WAIT:
      switch (word) {
        case COMMAND_FRAME:
          write_ptr = (uint8_t*) *get_back_buffer();
          frame_end = write_ptr + sizeof(frame_t);
          usart_state = USART_FRAME;
          break;
        case COMMAND_TEST_RING:
          usart_state = USART_TEST_RING;
          break;
        case COMMAND_TEST_SNAKE:
          usart_state = USART_TEST_SNAKE;
          break;
        case COMMAND_GET_ID:
          transmit_string("IT78:APA102:0", 13);
          break;
        default:
          break;
        }
      break;

    case USART_FRAME:
      *write_ptr = word;
      if (++write_ptr == frame_end) {
        flip_pages();
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

    // In case the state machine is in an undefined state, return to USART_WAIT.
    // This should not happen, but you never know...
    default:
      usart_state = USART_WAIT;
      break;

  }
}

