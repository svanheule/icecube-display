#include "render/test_ring.h"
#include "render/hex_geometry.h"
#include "frame_buffer.h"
#include "display_types.h"
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdlib.h>

#define DEFAULT_BRIGHTNESS 0x10

// Render expanding concentric rings around station 36 with coordinates (5,4)

static uint8_t ring_frame;

static void init_ring() {
  ring_frame = 0;
}

static void stop_ring() {}

struct frame_buffer_t* render_ring() {
  struct frame_buffer_t* frame = create_empty_frame();

  if (frame) {
    frame->flags = FRAME_FREE_AFTER_DRAW;

    // Only expand every second frame
    uint8_t radius = ring_frame>>1;

    uint8_t* buffer = frame->buffer;

    // Draw only IT78 stations, in-fill stations are already cleared
    for (uint8_t led = 0; led < LED_COUNT_IT78; ++led) {
      uint8_t d = get_string_distance_to_centre(led);
      uint16_t offset = sizeof(struct led_t)*led;
      // Set global brightness
      buffer[offset] = DEFAULT_BRIGHTNESS;

      // Set colour
      ++offset;
      for (uint8_t colour = 0; colour < 3; ++colour) {
        if (d+colour == radius) {
          buffer[offset+colour] = 0x0F;
        }
        else {
          buffer[offset+colour] = 0x00;
        }
      }
    }
  }

  // There are 6 rings available, but since there are two smaller circles 2 extra frames are needed
  // Count up 16 to halve the frame rate
  ring_frame = (ring_frame+1)%16;

  return frame;
}

static const struct renderer_t RING_RENDERER = {
    init_ring
  , stop_ring
  , render_ring
};

const struct renderer_t* get_ring_renderer() {
  return &RING_RENDERER;
}
