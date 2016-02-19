#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <stdint.h>
#include <stdlib.h>

#include "display_driver.h"
#include "display_state.h"
#include "frame_buffer.h"
#include "demo.h"
#include "test_render.h"
#include "remote.h"
#include "switches.h"

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

void pass() {}
struct frame_buffer_t* empty_frame() {
  struct frame_buffer_t* frame = create_frame();
  if (frame) {
    clear_frame(frame);
    frame->flags = FRAME_FREE_AFTER_DRAW;
  }
  return frame;
}

struct renderer_t display_idle = {
    pass
  , pass
  , empty_frame
};

const struct renderer_t* get_renderer(const enum display_state_t display_state) {
  switch (display_state) {
    case DISPLAY_DEMO:
      return get_demo_renderer();
      break;
    case DISPLAY_TEST_RING:
      return get_ring_renderer();
      break;
    case DISPLAY_TEST_SNAKE:
      return get_snake_renderer();
      break;
    case DISPLAY_IDLE:
      return &display_idle;
      break;
    case DISPLAY_EXTERNAL:
    default:
      return 0;
      break;
  }
}

int main () {
  // Init remote communications module
  init_remote();

  // Init pin configuration: USART, SPI
  init_display_driver();
  // USART port configuration is fixed by enabling USART Rx and Tx

  // Init timer configuration
  init_timer();
  init_switches();

  draw_frame = 0;

  // Enable idle mode sleep
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();

  // Enable interrupts
  sei();

  advance_display_state(DISPLAY_GOTO_IDLE);

  enum display_state_t state = get_display_state();
  const struct renderer_t* renderer = get_renderer(state);
  if (renderer) {
    renderer->start();
  }

  for (;;) {
    while (!draw_frame) {
      // Idle CPU until next interrupt
      sleep_cpu();
    }

    struct frame_buffer_t* frame = pop_frame();
    if (frame) {
      frame->flags |= FRAME_DRAW_IN_PROGRESS;
      display_frame((const frame_t*) frame->buffer);
      frame->flags &= ~FRAME_DRAW_IN_PROGRESS;
      if (frame->flags & FRAME_FREE_AFTER_DRAW) {
        free(frame);
      }
    }

    if ( get_display_state() == DISPLAY_IDLE
      && (switch_pressed(SWITCH_DEMO_1) || switch_pressed(SWITCH_DEMO_2))
    ) {
      clear_switch_pressed(SWITCH_DEMO_1);
      clear_switch_pressed(SWITCH_DEMO_2);
      advance_display_state(DISPLAY_GOTO_DEMO);
    }

    // Detect display state changes and switch renderers accordingly
    enum display_state_t new_state = get_display_state();
    if (state != new_state) {
      state = new_state;
      // Stop current renderer
      if (renderer) {
        renderer->stop();
      }
      // Select new renderer
      renderer = get_renderer(state);
      // Init new renderer
      if (renderer) {
        renderer->start();
      }
    }

    if (renderer) {
      push_frame(renderer->render_frame());
    }

    draw_frame = 0;
  }

  return 0;
}
