#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <stdint.h>

#include "display_driver.h"
#include "frame_buffer.h"
#include "test_render.h"
#include "serial.h"

/**
 * Dual frame buffer APA-102C LED display driver.
 * The frame_buffer_select variable indicates which frame buffer will be written out to the display.
 * The other buffer is then to be used by the USART interrupt handler to write the next frame to.
 * When the last byte of the next frame is written, the frame buffer select variable is updated
 * to indicate that a new frame is present.
 *
 * Frame transmission protocol requirements:
 * * Start-of-frame (SOF) indicator; to grab the frame buffer for writing
 * * 78 RGB 3-tuples (243 bytes) written to the buffer
 */

// CTC interrupt handling
volatile uint8_t draw_frame;

ISR(TIMER1_COMPA_vect) {
  // Trigger drawing of new frame
  draw_frame = 1;
}

void init_timer() {
  /* Clock is 16MHz
   * 25 FPS: 640000 counts; prescale 1024, compare (625-1)
   * 50 FPS: 320000 counts; prescale 256, compare (1250-1)
   * Prescale factor is 2^(n+2) with n = CS12:CS11:CS10
   * Set mode to 0100 : CTC with compare to OCR1A
   */
  OCR1A = 625-1;
  // Set compare mode, prescaler, and enable interrupt
  TCCR1B = (1<<WGM12) | (1<<CS12) | (1<<CS10);
  TIMSK1 = (1<<OCIE1A);
}

int main () {
  // Init USART configuration
  init_serial_port();

  // Init pin configuration: USART, SPI
  init_display_driver();
  // USART port configuration is fixed by enabling USART Rx and Tx

  // Init timer configuration
  init_timer();

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

    display_frame((const frame_t*) get_front_buffer());

    switch (get_usart_state()) {
      case USART_TEST_RING:
        render_ring(get_back_buffer());
        flip_pages();
        break;
      case USART_TEST_SNAKE:
        render_snake(get_back_buffer());
        flip_pages();
        break;
      default:
        break;
    }

    draw_frame = 0;
  }

  return 0;
}
