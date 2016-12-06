#ifndef SWITCHES_H
#define SWITCHES_H

/** \file
  * \brief Device hardware buttons interface.
  * \details The two control buttons present on the display (*Play/Pause* and *Forward*) can be
  *   used to control the display when in demo mode (see also demo.h).
  *   Instead of using pin-change interrupts, the switches are polled every 5ms to prevent
  *   bouncing from causing a lot of interrupts. Software debouncing is used ensure every button
  *   press only results in two edges that can be used by the firmware.
  *   To initialise the ports and timer, and be able to detect button presses, init_switches()
  *   should be called. switch_pressed() can then be used to check whether an up-going edge was
  *   detected. Down-going edges are currently ignored.
  *   When an up-going edge (after debouncing) was detected, the internal flag will remain set
  *   until clear_switch_pressed() is called with the appropriate switch index. Note that there
  *   is no time-out, so the internal flag will *not* be cleared automatically.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include <stdint.h>
#include <stdbool.h>

#if defined(CONTROLLER_ARDUINO)
#error Switch inputs currently not supported on the Arduino platform
#endif

/// Easy to remember name for the *Play/Pause* switch, instead of its button index 0.
#define SWITCH_PLAY_PAUSE 0

/// Easy to remember name for the *Forward* switch, instead of its button index 1.
#define SWITCH_FORWARD 1

/// Initialise hardware related to switches
void init_switches();

/// Read whether a switch press (up-going edge) has occured
bool switch_pressed(uint8_t switch_index);

/// Clear the switch-pressed flag
void clear_switch_pressed(uint8_t switch_index);

#endif
