#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <stdint.h>

#include "display_driver.h"
#include "display_properties.h"
#include "render/demo.h"
#include "render/test_scan.h"
#include "render/test_ring.h"
#include "render/boot_splash.h"
#include "remote.h"
#include "switches.h"
#include "frame_buffer.h"
#include "frame_queue.h"
#include "frame_timer.h"

enum display_state_t {
    DISPLAY_STATE_BOOT = 0
  , DISPLAY_STATE_IDLE
  , DISPLAY_STATE_BOOT_SPLASH
  , DISPLAY_STATE_EXTERNAL
  , DISPLAY_STATE_DEMO
  , DISPLAY_STATE_TEST_RING
  , DISPLAY_STATE_TEST_SCAN
};

static volatile enum display_state_t display_state = DISPLAY_STATE_BOOT;
static volatile const struct renderer_t* renderer = 0;
// If boot splash duration is > 0, display splash first.
// Otherwise go straight to idle.
static uint8_t boot_splash_duration = DEVICE_FPS-1;

static inline const struct renderer_t* get_renderer() {
  switch (display_state) {
    case DISPLAY_STATE_DEMO:
      return get_demo_renderer();
      break;
    case DISPLAY_STATE_TEST_RING:
      return get_ring_renderer();
      break;
    case DISPLAY_STATE_TEST_SCAN:
      return get_scan_renderer();
      break;
    case DISPLAY_STATE_BOOT_SPLASH:
      return get_boot_splash_renderer();
      break;
    case DISPLAY_STATE_BOOT:
    case DISPLAY_STATE_IDLE:
    case DISPLAY_STATE_EXTERNAL:
    default:
      return 0;
      break;
  }
}

static inline void advance_display_state() {
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
    renderer = get_renderer();
    // Init new renderer
    if (renderer) {
      if (renderer->start) {
        renderer->start();
      }
    }
    else {
      struct frame_buffer_t* f = create_empty_frame();
      if (f) {
        f->flags |= FRAME_FREE_AFTER_DRAW;
        if (!push_frame(f)) {
          destroy_frame(f);
        }
      }
    }
  }
}

static inline void consume_frame(struct frame_buffer_t* frame) {
  if (frame && frame->buffer) {
    display_frame(frame);
    if (frame->flags & FRAME_FREE_AFTER_DRAW) {
      destroy_frame(frame);
    }
  }
}

int main () {
  // Must be run *before* using any other display functions
  init_display_properties();
  // Initialise frame buffer memory before rendering
  init_frame_buffers();
  // Init display pin configuration and switches
  init_display_driver();
  display_blank();

  init_switches();

  // Enable idle mode sleep
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();

  // Init remote communications module
  init_remote();

  // Enable interrupts
  sei();

  // Init display timer just before display loop
  init_frame_timer();

  // Main loop
  for (;;) {
    while (!should_draw_frame()) {
      // Idle CPU until next interrupt
      sleep_cpu();
    }

    consume_frame(pop_frame());

    advance_display_state();

    if (renderer && !frame_queue_full()) {
      struct frame_buffer_t* f = renderer->render_frame();
      if (f && !push_frame(f) && (f->flags & FRAME_FREE_AFTER_DRAW)) {
        destroy_frame(f);
      }
    }

    clear_draw_frame();
  }

  return 0;
}
