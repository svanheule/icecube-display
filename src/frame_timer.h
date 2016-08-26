#ifndef FRAME_TIMER_H
#define FRAME_TIMER_H

/** \file
  * \brief Frame draw timing.
  * \details Timer to trigger drawing of a new frame 25 times per second.
  * \author Sander Vanheule (Universiteit Gent)
  */

/// Initialise the frame timer and set the function to be called when the timer trips.
void init_frame_timer(void (*timer_callback)());

#endif //FRAME_TIMER_H
