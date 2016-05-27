#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H

#include <stdbool.h>
#include "frame_buffer.h"

/// Check if the frame queue is full.
bool frame_queue_full();
/// Check if the frame queue is empty.
bool frame_queue_empty();

/** Push new frame into the frame FIFO.
  * \returns `true` on success, and `false` if the FIFO was full.
  */
bool push_frame(struct frame_buffer_t* frame);

/** Pop a frame from the frame FIFO.
  * \returns Pointer to the popped frame, or NULL if the FIFO was empty.
  */
struct frame_buffer_t* pop_frame();

#endif // FRAME_QUEUE_H
