#ifndef FRAME_TIMER_H
#define FRAME_TIMER_H

/** \file
  * \brief Frame draw timing.
  * \details Timer to trigger drawing of a new frame 25 times per second.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include <stdint.h>

/// Initialise the frame timer and set the function to be called when the timer trips.
void init_frame_timer(void (*timer_callback)());

void restart_frame_timer();

int8_t get_counter_direction();
uint32_t get_counts_max();
uint32_t get_counts_current();
void correct_counts_max(int32_t diff);

#endif //FRAME_TIMER_H
