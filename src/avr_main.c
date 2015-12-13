#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "display_driver.h"
#include "frame_buffer.h"

/**
 * Dual frame buffer APA-102C LED display driver.
 * The frame_buffer_select variable indicates which frame buffer will be written out to the display.
 * The other buffer is then to be used by the USART interrupt handler to write the next frame to.
 * When the last byte of the next frame is written, the frame buffer select variable is updated
 * to indicate that a new frame is present.
 *
 * Frame transmission protocol requirements:
 * * Start-of-frame (SOF) indicator; to grab the frame buffer for writing
 * * 81 RGB 3-tuples (243 bytes) written to the buffer
 */

// CTC interrupt handling
volatile unsigned char draw_frame;

ISR(TIMER1_COMPA_vect) {
  // Trigger drawing of new frame
  draw_frame = 1;
}


// USART interrupt handling
#define COMMAND_FRAME 0x01

enum UsartState_t {
    USART_WAIT
  , USART_FRAME
  // TODO Add diagnostics
};
typedef enum UsartState_t UsartState;

volatile UsartState usart_state;
volatile unsigned char bytes_remaining;

ISR(USART_RX_vect) {
  // Check status before reading word
  //const unsigned char status = UCSR0A;
  //if (status & (1<<FE0)) Frame error
  //if (status & (1<<DOR0)) Data overrun (FIFO full)
  //if (status & (1<<UPE0)) Parity error

  unsigned char word = UDR0;

  switch (usart_state) {
    case USART_WAIT:
      if (word == COMMAND_FRAME) {
        bytes_remaining = FRAME_LENGTH-1;
        usart_state = USART_FRAME;
      }
      break;

    case USART_FRAME:
      write_frame_byte(word);
      if (!(--bytes_remaining)) {
        usart_state = USART_WAIT;
      }
      break;

    default:
      break;
  }
  
}


int main () {
  // Init USART configuration
  // Set mode to async, 8 bit, no parity, 1 stop bit
  UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
  // Enable Rx, Tx, and Rx interrupts
  // Enable USART RX interrupts
  UCSR0B = (1<<RXCIE0) | (1<<RXEN0) | (1<<TXEN0);
  // Set baud rate to 19.2k, using 16MHz system clock
  // This is the fastest setting minicom was capable of that didn't procude garbage characters
  const unsigned int baud_rate_register = 51; // 16000000/(16*19200)-1;
  UBRR0H = (unsigned char) ((baud_rate_register >> 8) & 0x0F);
  UBRR0L = (unsigned char) baud_rate_register;

  usart_state = USART_WAIT;

  // Init pin configuration: USART, SPI
  // DDRB must be set before SPCR, so the internal pull-up doens't cause SPI to go into slave mode
  /* Configure port B as SPI master:
   * * B5: SCK (out)
   * * B4: MISO (in, unused)
   * * B3: MOSI (out)
   * * B2: /SS (out, unused)
   */
  DDRB &= (1<<DDB7) | (1<<DDB6); // Clear all but the clock pins
  DDRB |= (1<<DDB5) | (1<<DDB3) | (1<<DDB2);
  init_driver();
  // USART port configuration is fixed by enabling USART Rx and Tx

  init_frame_buffer();

  // Init counter configuration
  /* Clock is 16MHz
   * 25 FPS: 640000 counts; prescale 1024, compare (625-1)
   * 50 FPS: 320000 counts; prescale 256, compare (1250-1)
   * Prescale factor is 8*2^(n-1) with n = CS12:CS11:CS10
   * Set mode to 0100 : CTC with compare to OCR1A
   */
  OCR1A = 625-1;
  // Set compare mode, prescaler, and enable interrupt
  TCCR1B = (1<<WGM12) | (1<<CS12) | (1<<CS10);
  TIMSK1 = (1<<OCIE1A);

  draw_frame = 0;

  // Enable idle mode sleep
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();

  // Enable interrupts
  sei();

  for (;;) {
    while (!draw_frame) {
      // Idle CPU until next interrupt
      sleep_cpu();
    }

    display_frame(get_frame_buffer());
    draw_frame = 0;
  }

  return 0;
}
