#ifndef TEST_RENDER_H
#define TEST_RENDER_H

#include "frame_buffer.h"

/// Render a number of pixels bouncing back and forth between the start and end of the buffer
void render_snake(frame_t* buffer);

/// Render consecutive concentric rings around pixel (5,4), i.e. station 36
void render_ring(frame_t* buffer);

#endif
