#include "render/test.h"
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdlib.h>

#define DEFAULT_BRIGHTNESS 0x10

// Render pixel trails
#define TAIL_LENGTH 3
static uint8_t scan_frame;

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
  struct frame_buffer_t* frame = create_frame();

  if (frame) {
    frame->flags = FRAME_FREE_AFTER_DRAW;
    clear_frame(frame);
    struct led_t* write_ptr = &frame->buffer[0];

    uint8_t tail = TAIL_LENGTH;
    while(tail--) {
      write_ptr[scan_frame+tail] = (struct led_t) {DEFAULT_BRIGHTNESS, 0x10, 0x10, 0x10};
    }

    if ((scan_frame == LED_COUNT-TAIL_LENGTH) && (direction == COUNT_UP)) {
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

// Render expanding concentric rings around station 36 with coordinates (5,4)
struct station_t {
  int8_t v;
  int8_t w;
};
static const struct station_t STATION_VECTOR[LED_COUNT] PROGMEM = {
    {0,0}, {1,0}, {2,0}, {3,0}, {4,0}, {5,0}
  , {0,1}, {1,1}, {2,1}, {3,1}, {4,1}, {5,1}, {6,1}
  , {0,2}, {1,2}, {2,2}, {3,2}, {4,2}, {5,2}, {6,2}, {7,2}
  , {0,3}, {1,3}, {2,3}, {3,3}, {4,3}, {5,3}, {6,3}, {7,3}, {8,3}
  , {0,4}, {1,4}, {2,4}, {3,4}, {4,4}, {5,4}, {6,4}, {7,4}, {8,4}, {9,4}
  , {1,5}, {2,5}, {3,5}, {4,5}, {5,5}, {6,5}, {7,5}, {8,5}, {9,5}, {10,5}
  , {2,6}, {3,6}, {4,6}, {5,6}, {6,6}, {7,6}, {8,6}, {9,6}, {10,6}
  , {3,7}, {4,7}, {5,7}, {6,7}, {7,7}, {8,7}, {9,7}, {10,7}
  , {4,8}, {5,8}, {6,8}, {7,8}, {8,8}, {9,8}, {10,8}
  , {5,9}, {6,9}, {7,9}, {8,9}
};
static const struct station_t CENTRE = {5,4};

static uint8_t ring_frame;

static void init_ring() {
  ring_frame = 0;
}

static void stop_ring() {}

/* Extensive explanation at: [http://www.redblobgames.com/grids/hexagons/]
 * A hexagonal lattice may be constructed from a regular, 3D square grid by taking the plane
 * satisfying the eqaution \f$x+y+z=0\f$. If the pair \f$(x,y)\f$ is mapped onto a 2D space
 * given by \f$(v,w)\f$, this plane equation determines \f$z\f$.
 * The distance between 2 points on a hexagonal is given by
 * \f$0.5 (|x_1-x_2| + |y_1-y_2| + |z_1-z_2|)\f$ or
 * \f$0.5 (|v_1-v_2| + |w_1-w_2| + |(v_1-v_2)-(w_1-w_2)|)\f$.
 */
static uint8_t station_distance(const struct station_t s1, const struct station_t s2) {
  int8_t dv = s1.v - s2.v;
  int8_t dw = s1.w - s2.w;
  return (abs(dv) + abs(dw) + abs(dv-dw))>>1;
}

struct frame_buffer_t* render_ring() {
  struct frame_buffer_t* frame = create_frame();

  if (frame) {
    frame->flags = FRAME_FREE_AFTER_DRAW;
    clear_frame(frame);

    // Only expand every second frame
    uint8_t radius = ring_frame>>1;

    uint8_t* buffer = (uint8_t*) frame->buffer;

    for (uint8_t led = 0; led < LED_COUNT; ++led) {
      struct station_t station;
      memcpy_P(&station, STATION_VECTOR+led, sizeof(struct station_t));
      uint8_t d = station_distance(station, CENTRE);
      uint16_t offset = 4*led;
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
