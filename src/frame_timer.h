#ifndef FRAME_TIMER_H
#define FRAME_TIMER_H

/** \file
  * \brief Frame draw timing.
  * \details Timer to trigger drawing of a new frame 25 times per second.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include <stdint.h>
#include <stdbool.h>

#if defined(__MK20DX256__)
typedef uint32_t timer_count_t;
typedef int32_t timer_diff_t;
#else
typedef uint16_t timer_count_t;
typedef int16_t timer_diff_t;
#endif

/// Initialise the frame timer and set the function to be called when the timer trips.
void init_frame_timer(void (*timer_callback)());

int8_t get_counter_direction();
timer_count_t get_counts_max();
timer_count_t get_counts_current();
void correct_counts_max(timer_diff_t diff);

#endif //FRAME_TIMER_H
