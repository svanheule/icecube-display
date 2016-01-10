#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>
#include "frame_buffer.h"

#define PULSE_DURATION 33 // In number of frames (25 fps), must be smaller than 128 (int8_t)

struct pulse_t {
  uint16_t time; //< Turn-on time of led
  uint8_t led_index;
  struct led_t led; //< LED brightness and colour
};

struct event {
  uint16_t length; //< Number of pulses in this event
  struct pulse_t* pulses; //< Array of pulses
};

#endif //EVENT_H
