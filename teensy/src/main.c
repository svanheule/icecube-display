#include "frame_timer.h"
#include "frame_buffer.h"
#include "frame_queue.h"
#include "display_driver.h"

#include "kinetis/io.h"
#include "remote.h"

#include "render/test.h"

// Display state
enum display_state_t {
    DISPLAY_STATE_BOOT = 0
  , DISPLAY_STATE_IDLE
  , DISPLAY_STATE_BOOT_SPLASH
  , DISPLAY_STATE_EXTERNAL
  , DISPLAY_STATE_DEMO
  , DISPLAY_STATE_TEST_RING
  , DISPLAY_STATE_TEST_SNAKE
};

static volatile enum display_state_t display_state = DISPLAY_STATE_BOOT;
static const struct renderer_t* volatile renderer = 0;
// If boot splash duration is > 0, display splash first.
// Otherwise go straight to idle.
static uint8_t boot_splash_duration = 24;

static inline const struct renderer_t* get_renderer() {
  switch (display_state) {
#ifdef DEVICE_TEST_MODE
    case DISPLAY_STATE_IDLE:
#endif
    case DISPLAY_STATE_TEST_SNAKE:
      return get_test_renderer();
      break;
    case DISPLAY_STATE_DEMO:
/*      return get_demo_renderer();*/
/*      break;*/
    case DISPLAY_STATE_TEST_RING:
/*      return get_ring_renderer();*/
/*      break;*/
    case DISPLAY_STATE_BOOT_SPLASH:
/*      return get_boot_splash_renderer();*/
/*      break;*/
    case DISPLAY_STATE_BOOT:
#ifndef DEVICE_TEST_MODE
    case DISPLAY_STATE_IDLE:
#endif
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
  else if (display_state == DISPLAY_STATE_IDLE && is_remote_connected()) {
    new_state = DISPLAY_STATE_EXTERNAL;
  }
  else if (display_state == DISPLAY_STATE_EXTERNAL && !is_remote_connected()) {
    new_state = DISPLAY_STATE_IDLE;
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


// Frame draw timer
volatile uint8_t draw_frame;

void frame_timer_callback() {
  // Trigger drawing of new frame
  draw_frame = 1;
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
  init_display_buffers();

  // Init display pin configuration
  init_display_driver();
  display_blank();

  // Init remote communications module
  init_remote();

  // Init display timer just before display loop
  draw_frame = 0;
  init_frame_timer(frame_timer_callback);

  // Main loop
  for (;;) {
    while (!draw_frame) {
      // Idle CPU until next interrupt
      asm("wfi");
    }

    consume_frame(pop_frame());

    advance_display_state();

    if (renderer && !frame_queue_full()) {
      struct frame_buffer_t* f = renderer->render_frame();
      if (f && !push_frame(f) && (f->flags & FRAME_FREE_AFTER_DRAW)) {
        destroy_frame(f);
      }
    }

    draw_frame = 0;
  }

  return 0;
}
