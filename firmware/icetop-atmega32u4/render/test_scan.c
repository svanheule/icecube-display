#include "render/test_scan.h"
#include "frame_buffer.h"
#include <stdint.h>
#include <stdlib.h>

#define DEFAULT_BRIGHTNESS 0x10

// Render pixel trails
#define TAIL_LENGTH 3
static uint16_t scan_frame;

enum direction_t {
    COUNT_UP = 1
  , COUNT_DOWN = -1
};
static enum direction_t direction;

static void init_scan() {
  direction = COUNT_UP;
  scan_frame = 0;
}

static void stop_scan() {}

struct frame_buffer_t* render_scan() {
  struct frame_buffer_t* frame = create_empty_frame();

  if (frame) {
    frame->flags = FRAME_FREE_AFTER_DRAW;
    struct led_t* write_ptr = &frame->buffer[0];

    uint8_t tail = TAIL_LENGTH;
    while(tail--) {
      write_ptr[scan_frame+tail] = (struct led_t) {DEFAULT_BRIGHTNESS, 0x10, 0x10, 0x10};
    }

    const uint16_t led_count = get_led_count();
    if ((scan_frame == led_count-TAIL_LENGTH) && (direction == COUNT_UP)) {
      direction = COUNT_DOWN;
    }
    else if ((scan_frame == 0) && (direction == COUNT_DOWN)) {
      direction = COUNT_UP;
    }
  }

  scan_frame = scan_frame + direction;

  return frame;
}

static const struct renderer_t SCAN_RENDERER = {
    init_scan
  , stop_scan
  , render_scan
};

const struct renderer_t* get_scan_renderer() {
  return &SCAN_RENDERER;
}
