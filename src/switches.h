#ifndef SWITCHES_H
#define SWITCHES_H

#include <stdint.h>
#include <stdbool.h>

#if defined(CONTROLLER_ARDUINO)
#error Switch inputs currently not supported on the Arduino platform
#endif

#define SWITCH_PLAY_PAUSE 0
#define SWITCH_FORWARD 1

/// Initialise hardware related to switches
void init_switches();

/// Read whether a switch press (down-going) edge has occured
bool switch_pressed(uint8_t switch_index);

/// Clear the switch-pressed flag
void clear_switch_pressed(uint8_t switch_index);

#endif
