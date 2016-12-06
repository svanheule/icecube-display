#ifndef FRAME_TIMER_BACKEND_H
#define FRAME_TIMER_BACKEND_H

/** \file
  * \brief Frame draw timing backend.
  * \details Platform specific implementation of the frame timer.
  *   The frame draw frequency can be changed with correct_counts_max(), which modifies the
  *   frame draw interval. This allows for small corrections to be made such that multiple
  *   devices can run at approximately the same frame draw frequency.
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
void init_frame_timer_backend(void (*timer_callback)());

/** \brief Counter direction.
  * \details Differences between counter values have the same sign as the value returned by
  *   this function, except when the timer has rolled over.
  * \return 1 for up-counter, -1 for down-counter.
  */
int8_t get_counter_direction();

/// The maximum (roll-over/reset) value of the timer.
timer_count_t get_counts_max();

/// Get the current counter value of the timer.
timer_count_t get_counts_current();

/** \brief Add \a diff to the current maximum value of the counter.
  * \details The new value will be applied after the counter has rolled over.
  *   Therefor, although possible, it doesn't really make sense to call this function more
  *   often than once per frame draw cycle.
  */
void correct_counts_max(timer_diff_t diff);

#endif //FRAME_TIMER_BACKEND_H