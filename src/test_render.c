#include "test_render.h"

#define DEFAULT_BRIGHTNESS 0x10

// Render snaking pixel trail
#define TAIL_LENGTH 3
static unsigned char snake_frame;

enum direction_t {
    COUNT_UP = 1
  , COUNT_DOWN = -1
};
static enum direction_t direction = COUNT_UP;


void render_snake(frame_t* buffer) {
  clear_frame(buffer);
  struct led_t* write_ptr = *buffer;

  unsigned char tail = TAIL_LENGTH;
  while(tail--) {
    write_ptr[snake_frame+tail] = (struct led_t) {DEFAULT_BRIGHTNESS, 0x10, 0x10, 0x10};
  }

  if ((snake_frame == LED_COUNT-TAIL_LENGTH) && (direction == COUNT_UP)) {
    direction = COUNT_DOWN;
  }
  else if ((snake_frame == 0) && (direction == COUNT_DOWN)) {
    direction = COUNT_UP;
  }

  snake_frame = snake_frame + direction;
}


// Render expanding concentric rings around station 36 with coordinates (5,4) (pixel nr. 35)
static const signed char STATION_VECTOR[LED_COUNT][2] = {
    {5,0}, {4,0}, {3,0}, {2,0}, {1,0}, {0,0}
  , {0,1}, {1,1}, {2,1}, {3,1}, {4,1}, {5,1}, {6,1}
  , {7,2}, {6,2}, {5,2}, {4,2}, {3,2}, {2,2}, {1,2}, {0,2}
  , {0,3}, {1,3}, {2,3}, {3,3}, {4,3}, {5,3}, {6,3}, {7,3}, {8,3}
  , {9,4}, {8,4}, {7,4}, {6,4}, {5,4}, {4,4}, {3,4}, {2,4}, {1,4}, {0,4}
  , {1,5}, {2,5}, {3,5}, {4,5}, {5,5}, {6,5}, {7,5}, {8,5}, {9,5}, {10,5}
  , {10,6}, {9,6}, {8,6}, {7,6}, {6,6}, {5,6}, {4,6}, {3,6}, {2,6}
  , {3,7}, {4,7}, {5,7}, {6,7}, {7,7}, {8,7}, {9,7}, {10,7}
  , {10,8}, {9,8}, {8,8}, {7,8}, {6,8}, {5,8}, {4,8}
  , {5,9}, {6,9}, {7,9}, {8,9}
};
static char ring_frame;
typedef signed char Station[2];

static unsigned char station_distance(Station s1, Station s2) {
  signed char dv = s1[0]-s2[0];
  signed char dw = s1[1]-s2[1];
  return (abs(dv) + abs(dw) + abs(dv-dw))>>1;
}

void render_ring(frame_t* buffer) {
  // Only expand every second frame
  unsigned char radius = ring_frame>>1;

  uint8_t* frame = (uint8_t*) *buffer;

  unsigned char station, d, offset, colour;
  for (station = 0; station < LED_COUNT; ++station) {
    d = station_distance(STATION_VECTOR[station], STATION_VECTOR[34]);
    offset = 4*station;
    frame[offset] = DEFAULT_BRIGHTNESS;
    ++offset;

    for (colour = 0; colour < 3; ++colour) {
      if (d+colour == radius) {
        frame[offset+colour] = 0x0F;
      }
      else {
        frame[offset+colour] = 0x00;
      }
    }
  }

  // There are 6 rings available, but since there are two smaller circles 2 extra frames are needed
  // Count up 16 to halve the frame rate
  ring_frame = (ring_frame+1)%16;
}
