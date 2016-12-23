#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H

/** \file
  * \brief Display frame queue.
  * \details A flexible display system is created by using a first-in, first-out queue which hold
  *   pointers to frame buffer objects.
  *   Multiple instances can push frames in the queue to be displayed.
  *   They can do this independently from each other, without running the risk of (partially)
  *   overwriting an existing frame, or a frame that is currently being drawn.
  *   Queue manipulation is done atomically to ensure no pointers are dropped, or invalid pointers
  *   are returned.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include <stdbool.h>
#include "frame_buffer.h"

/// \name Frame queue manipulation
/// @{

/// Check if the frame queue is full.
/// \ingroup led_display_buffer
bool frame_queue_full();
/// Check if the frame queue is empty.
/// \ingroup led_display_buffer
bool frame_queue_empty();

/** Push new frame into the frame FIFO.
  * \returns `true` on success, and `false` if the FIFO was full.
  * \ingroup led_display_buffer
  */
bool push_frame(struct frame_buffer_t* frame);

/** Pop a frame from the frame FIFO.
  * \returns Pointer to the popped frame, or NULL if the FIFO was empty.
  * \ingroup led_display_buffer
  */
struct frame_buffer_t* pop_frame();

/// @}

#endif // FRAME_QUEUE_H
