#ifndef RENDER_TEST_H
#define RENDER_TEST_H

#include "frame_buffer.h"

/// Render a number of pixels scanning from the start to end of the buffer, and back.
const struct renderer_t* get_scan_renderer();

/// Render consecutive concentric rings around pixel (5,4), i.e. station 36
const struct renderer_t* get_ring_renderer();

#endif
