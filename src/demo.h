#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>
#include "frame_buffer.h"

#define PULSE_DURATION 33 // In number of frames (25 fps), must be smaller than 128 (int8_t)
#define CLEAR_DURATION 50

struct pulse_t {
  uint16_t time; //< Turn-on time of led
  uint8_t led_index;
  struct led_t led; //< LED brightness and colour
};

struct event_t {
  uint16_t length; //< Number of pulses in this event
  const struct pulse_t* pulses; //< Array of pulses
};

void init_demo();
uint8_t demo_finished();
void render_demo(frame_t* buffer);

#endif //EVENT_H
