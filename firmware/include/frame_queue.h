#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H

/** \file
  * \brief Display frame queue.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include <stdbool.h>
#include "frame_buffer.h"

/** \defgroup led_display_queue Display frame queue
  * \ingroup led_display
  * \brief FIFO interface for frame displaying.
  * \details A flexible display system is created by using a first-in, first-out queue which holds
  *   pointers to frame buffer objects.
  *   Multiple instances can push frames in the queue to be displayed.
  *   They can do this independently from each other, without running the risk of (partially)
  *   overwriting an existing frame, or a frame that is currently being drawn.
  *   Queue manipulation is done atomically to ensure no pointers are dropped, or invalid pointers
  *   are returned.
  * @{
  * \name Frame queue manipulation
  * @{
  */

/// \brief Check if the frame queue is full.
bool frame_queue_full();
/// \brief Check if the frame queue is empty.
bool frame_queue_empty();

/// \brief Push new frame into the frame FIFO.
/// \returns `true` on success, and `false` if the FIFO was full.
bool push_frame(struct frame_buffer_t* frame);

/// \brief Pop a frame from the frame FIFO.
/// \returns Pointer to the popped frame, or NULL if the FIFO was empty.
struct frame_buffer_t* pop_frame();

/// @}
/// @}

#endif // FRAME_QUEUE_H
