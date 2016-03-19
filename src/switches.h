#ifndef SWITCHES_H
#define SWITCHES_H

#include <stdint.h>
#include <stdbool.h>

#define SWITCH_COUNT 2
#define SWITCH_PLAY_PAUSE 0
#define SWITCH_FORWARD 1

/// Initialise hardware related to switches
void init_switches();

/// Read whether a switch press (down-going) edge has occured
bool switch_pressed(uint8_t switch_index);

/// Clear the switch-pressed flag
void clear_switch_pressed(uint8_t switch_index);

#endif
