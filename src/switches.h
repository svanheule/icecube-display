#ifndef SWITCHES_H
#define SWITCHES_H

#include <stdint.h>

#define SWITCH_COUNT 1
#define SWITCH_DEMO_1 0
//#define SWITCH_DEMO_2 1

enum switch_state_t {
    PRESSED
  , DEPRESSED
};

/// Initialise hardware related to switches
void init_switches();

/// Read whether a switch press (down-going) edge has occured
uint8_t switch_pressed(uint8_t switch_index);

/// Clear the switch-pressed flag
void clear_switch_pressed(uint8_t switch_index);

#endif
