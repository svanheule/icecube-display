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

#ifdef __DOXYGEN__
/** \brief Frame timer counter resolution in bits.
  * \details Preferably supplied as a compiler flag.
  *   If not defined, a warning will be issued and the timer resolution will be assumed to be equal
  *   to the width of `int` on the platform.
  */
#define FRAME_TIMER_RESOLUTION
#elif !defined(FRAME_TIMER_RESOLUTION)
#warning FRAME_TIMER_RESOLUTION not defined
typedef unsigned int timer_count_t;
typedef signed int timer_diff_t;
#elif FRAME_TIMER_RESOLUTION < 32
typedef uint64_t timer_count_t;
typedef int64_t timer_diff_t;
#elif FRAME_TIMER_RESOLUTION < 16
typedef uint32_t timer_count_t;
typedef int32_t timer_diff_t;
#else
typedef uint16_t timer_count_t;
typedef int16_t timer_diff_t;
#endif

/// \addtogroup led_display_timer
/// @{
/// \name Frame timer backend
/// @{

/// \brief Initialise the frame timer.
/// \param timer_callback The function to be called when the timer trips.
void init_frame_timer_backend(void (*timer_callback)());

/** \brief Counter direction.
  * \details Differences between counter values have the same sign as the value returned by
  *   this function, except when the timer has rolled over.
  * \return 1 for up-counter, -1 for down-counter.
  */
int8_t get_counter_direction();

/// \brief The maximum (roll-over/reset) value of the timer.
timer_count_t get_counts_max();

/// \brief Get the current counter value of the timer.
timer_count_t get_counts_current();

/** \brief Add \a diff to the current maximum value of the counter.
  * \details The new value will be applied after the counter has rolled over.
  *   Therefore, although possible, it doesn't really make sense to call this function more
  *   often than once per frame draw cycle.
  */
void correct_counts_max(timer_diff_t diff);

/// @}
/// @}

#endif //FRAME_TIMER_BACKEND_H
