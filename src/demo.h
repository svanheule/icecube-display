#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>
#include "frame_buffer.h"

// In number of frames (25 fps), must be smaller than 256 (uint8_t)
#define PULSE_DURATION 25
#define PULSE_CLEAR_DURATION 33
#define OVERVIEW_DURATION 75
#define OVERVIEW_CLEAR_DURATION 50

struct pulse_t {
  uint16_t time; //< Turn-on time of led
  uint8_t led_index;
  struct led_t led; //< LED brightness and colour
};

void init_demo();
uint8_t demo_finished();
void render_demo(frame_t* buffer);

#endif //EVENT_H
