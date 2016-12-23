#include "render/rain.h"
#include "frame_buffer.h"
#include "display_types.h"
#include <stdint.h>
#include "usb/led.h"

// Render pixel trails
#define DOM_SPACING 10
static uint8_t color;
static uint8_t dom_index;

static void init_rain() {
  color = 0;
  dom_index = 0;
}

static void stop_rain() {}

struct frame_buffer_t* render_rain() {
  struct frame_buffer_t* frame = create_empty_frame();

  if (frame) {
    frame->flags = FRAME_FREE_AFTER_DRAW;
    uint8_t* output = frame->buffer;

    uint8_t string_count = get_led_count()/60;

    for (unsigned string = 0; string < string_count; string++) {
      for (unsigned dom = dom_index; dom < 60; dom+=DOM_SPACING) {
        ptrdiff_t buffer_offset = (string*60 + dom)*sizeof(struct led_t);
        output[buffer_offset+color] = (1<<4);
      }
    }
  }

  if (color == 2) {
    dom_index = (dom_index + 1)%DOM_SPACING;
  }
  color = (color+1)%3;

  return frame;
}

static const struct renderer_t RAIN_RENDERER = {
    init_rain
  , stop_rain
  , render_rain
};

const struct renderer_t* get_rain_renderer() {
  return &RAIN_RENDERER;
}

