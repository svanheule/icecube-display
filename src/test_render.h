#ifndef TEST_RENDER_H
#define TEST_RENDER_H

#include "frame_buffer.h"

/// Render a number of pixels bouncing back and forth between the start and end of the buffer
const struct renderer_t* get_snake_renderer();

/// Render consecutive concentric rings around pixel (5,4), i.e. station 36
const struct renderer_t* get_ring_renderer();

#endif
