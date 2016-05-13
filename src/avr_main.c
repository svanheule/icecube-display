#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <stdint.h>

#include "display_driver.h"
#include "display_state.h"
#include "render/demo.h"
#include "render/test.h"
#include "render/boot_splash.h"
#include "remote.h"
#include "switches.h"

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

struct frame_buffer_t* empty_frame() {
  struct frame_buffer_t* frame = create_frame();
  if (frame) {
    clear_frame(frame);
    frame->flags = FRAME_FREE_AFTER_DRAW;
  }
  return frame;
}

void pop_and_display_frame() {
  struct frame_buffer_t* frame = pop_frame();
  if (frame) {
    frame->flags |= FRAME_DRAW_IN_PROGRESS;
    display_frame((const frame_t*) frame->buffer);
    frame->flags &= ~FRAME_DRAW_IN_PROGRESS;
    destroy_frame(frame);
  }
}

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
    case DISPLAY_BOOT_SPLASH:
      return get_boot_splash_renderer();
      break;
    case DISPLAY_IDLE:
    case DISPLAY_EXTERNAL:
    default:
      return 0;
      break;
  }
}

int main () {
  // Init display pin configuration and switches
  init_display_driver();
  init_switches();

  draw_frame = 0;

  // Enable idle mode sleep
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();

  // Init remote communications module
  init_remote();

  // Enable interrupts
  sei();

  advance_display_state(DISPLAY_GOTO_BOOT_SPLASH);

  // If the current state already has an associated renderer, render and push first frame.
  // Otherwise clear the display.
  enum display_state_t state = get_display_state();
  const struct renderer_t* renderer = get_renderer(state);
  struct frame_buffer_t* frame;
  if (renderer && renderer->start) {
    renderer->start();
    frame = renderer->render_frame();
  }
  else {
    frame = empty_frame();
  }

  if (!push_frame(frame)) {
    destroy_frame(frame);
  }

  // Init display timer just before display loop
  init_timer();

  // Boot splash loop
  uint8_t frame_counter = 0;
  while (state == DISPLAY_BOOT_SPLASH) {
    while(!draw_frame) {
      sleep_cpu();
    }

    pop_and_display_frame();

    if (frame_counter < 2*25) {
      frame = renderer->render_frame();
      ++frame_counter;
    }
    else {
      frame = empty_frame();
      advance_display_state(DISPLAY_GOTO_IDLE);
    }

    if (!push_frame(frame)) {
      destroy_frame(frame);
    }

    state = get_display_state();
    renderer = get_renderer(state);

    draw_frame = 0;
  }

  // Main loop
  for (;;) {
    while (!draw_frame) {
      // Idle CPU until next interrupt
      sleep_cpu();
    }

    pop_and_display_frame();

    if (!is_remote_connected()) {
      if ( state == DISPLAY_IDLE
        && (switch_pressed(SWITCH_PLAY_PAUSE) || switch_pressed(SWITCH_FORWARD))
      ) {
        advance_display_state(DISPLAY_GOTO_DEMO);
      }
      else if (state == DISPLAY_EXTERNAL) {
        // Clear any switch presses on going to IDLE
        clear_switch_pressed(SWITCH_PLAY_PAUSE);
        clear_switch_pressed(SWITCH_FORWARD);
        advance_display_state(DISPLAY_GOTO_IDLE);
      }
    }
    else if (state != DISPLAY_EXTERNAL) {
      advance_display_state(DISPLAY_GOTO_IDLE);
      // TODO push cleared frame
      advance_display_state(DISPLAY_GOTO_EXTERNAL);
    }

    enum display_state_t current_state = get_display_state();
    // Detect display state changes and switch renderers accordingly
    if (state != current_state) {
      state = current_state;
      // Stop current renderer
      if (renderer && renderer->stop) {
        renderer->stop();
      }
      // Select new renderer
      renderer = get_renderer(state);
      // Init new renderer
      if (renderer && renderer->start) {
        renderer->start();
      }
      else if (!frame_queue_full()) {
        push_frame(empty_frame());
      }
    }

    if (renderer && !frame_queue_full()) {
      push_frame(renderer->render_frame());
    }

    draw_frame = 0;
  }

  return 0;
}
