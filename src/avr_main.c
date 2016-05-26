#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <stdint.h>

#include "display_driver.h"
//#include "display_state.h"
#include "render/demo.h"
#include "render/test.h"
#include "render/boot_splash.h"
#include "remote.h"
#include "switches.h"

enum display_state_t {
    DISPLAY_STATE_BOOT = 0
  , DISPLAY_STATE_IDLE
  , DISPLAY_STATE_BOOT_SPLASH
  , DISPLAY_STATE_EXTERNAL
  , DISPLAY_STATE_DEMO
  , DISPLAY_STATE_TEST_RING
  , DISPLAY_STATE_TEST_SNAKE
};

const struct renderer_t* get_renderer(const enum display_state_t display_state) {
  switch (display_state) {
    case DISPLAY_STATE_DEMO:
      return get_demo_renderer();
      break;
    case DISPLAY_STATE_TEST_RING:
      return get_ring_renderer();
      break;
    case DISPLAY_STATE_TEST_SNAKE:
      return get_snake_renderer();
      break;
    case DISPLAY_STATE_BOOT_SPLASH:
      return get_ring_renderer();
      break;
    case DISPLAY_STATE_BOOT:
    case DISPLAY_STATE_IDLE:
    case DISPLAY_STATE_EXTERNAL:
    default:
      return 0;
      break;
  }
}

struct frame_buffer_t* empty_frame() {
  struct frame_buffer_t* frame = create_frame();
  if (frame) {
    clear_frame(frame);
    frame->flags = FRAME_FREE_AFTER_DRAW;
  }
  return frame;
}

static enum display_state_t display_state = DISPLAY_STATE_BOOT;
static const struct renderer_t* renderer = 0;
// If boot splash duration is > 0, display splash first.
// Otherwise go straight to idle.
static uint8_t boot_splash_duration = 15;

void advance_display_state() {
  // By default, keep old state
  enum display_state_t new_state = display_state;

  if (display_state == DISPLAY_STATE_BOOT) {
    if (boot_splash_duration > 0) {
      new_state = DISPLAY_STATE_BOOT_SPLASH;
    }
    else {
      new_state = DISPLAY_STATE_IDLE;
    }
  }
  else if (display_state == DISPLAY_STATE_BOOT_SPLASH) {
    if (boot_splash_duration > 0) {
      --boot_splash_duration;
    }
    else {
      new_state = DISPLAY_STATE_IDLE;
    }
  }
  else if (!is_remote_connected()) {
    if ( display_state == DISPLAY_STATE_IDLE
      && (switch_pressed(SWITCH_PLAY_PAUSE) || switch_pressed(SWITCH_FORWARD))
    ) {
      new_state = DISPLAY_STATE_DEMO;
    }
    else if (display_state == DISPLAY_STATE_EXTERNAL) {
      // Clear any switch presses on going to IDLE
      clear_switch_pressed(SWITCH_PLAY_PAUSE);
      clear_switch_pressed(SWITCH_FORWARD);
      new_state = DISPLAY_STATE_IDLE;
    }
  }
  else if (display_state != DISPLAY_STATE_EXTERNAL) {
    new_state = DISPLAY_STATE_EXTERNAL;
  }

  // Check if state changed and switch renderers accordingly
  if (new_state != display_state) {
    // Stop current renderer
    if (renderer && renderer->stop) {
      renderer->stop();
    }
    // Select new renderer
    display_state = new_state;
    renderer = get_renderer(new_state);
    // Init new renderer
    if (renderer) {
      if (renderer->start) {
        renderer->start();
      }
    }
    else {
      struct frame_buffer_t* f = empty_frame();
      if (!push_frame(f)) {
        destroy_frame(f);
      }
    }
  }
}

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

void consume_frame(struct frame_buffer_t* frame) {
  if (frame) {
    display_frame(frame);
    if (frame->flags & FRAME_FREE_AFTER_DRAW) {
      destroy_frame(frame);
    }
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

  // Clear display just in case the LEDs didn't power on without output
  consume_frame(empty_frame());

  // Initialise state and renderer variables

  // Init display timer just before display loop
  init_timer();

  // Main loop
  for (;;) {
    while (!draw_frame) {
      // Idle CPU until next interrupt
      sleep_cpu();
    }

    consume_frame(pop_frame());

    advance_display_state();

    if (renderer && !frame_queue_full()) {
      struct frame_buffer_t* f = renderer->render_frame();
      if (!push_frame(f) && (f->flags & FRAME_FREE_AFTER_DRAW)) {
        destroy_frame(f);
      }
    }

    draw_frame = 0;
  }

  return 0;
}
