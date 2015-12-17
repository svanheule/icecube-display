#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include "frame_buffer.h"

// Initialise registers required for driving the display
void init_driver();

// Write a frame out to the display from the given frame buffer
void display_frame(const frame_t* buffer);

#endif
