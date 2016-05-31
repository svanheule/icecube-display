#ifndef RENDER_TEST_H
#define RENDER_TEST_H

/** \file
  * \brief Simple renderers that run on the microcontroller to test the display.
  * \details Two test renderers are currently defined:
  *   * Line scanner: Generates three white pixels that slide through the buffer, effectively
  *       scanning line by line.
  *   * Ring renderer: Generates concentric red, green, and blue rings expanding out from
  *       the centre of the display.
  *
  * \author Sander Vanheule (Universiteit Gent)
  */

#include "render/renderer.h"

/// Render a number of pixels scanning from the start to end of the buffer, and back.
const struct renderer_t* get_scan_renderer();

/// Render consecutive concentric rings around station 36.
const struct renderer_t* get_ring_renderer();

#endif
