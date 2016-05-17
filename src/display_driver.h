#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include "frame_buffer.h"

// Initialise registers required for driving the display
void init_display_driver();

/// Write a frame out to the display from the given frame buffer.
/// During the write-out, the `FRAME_DRAW_IN_PROGRESS` flag will be set on the frame.
void display_frame(struct frame_buffer_t* buffer);

#endif
