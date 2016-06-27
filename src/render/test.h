#ifndef RENDER_TEST_H
#define RENDER_TEST_H

/** \file
  * \brief Simple renderers that run on the microcontroller to test the display.
  * \details Two test renderers are currently defined:
  *   * Line scanner: get_scan_renderer()
  *   * Ring renderer: get_ring_renderer()
  *
  * \author Sander Vanheule (Universiteit Gent)
  */

#include "render/renderer.h"

/** \brief Render a number of pixels scanning from the start to end of the buffer, and back.
  * \details Generates three white pixels that slide through the buffer, effectively
  *   scanning the display line by line.
  * \ingroup display_renderer
  */
const struct renderer_t* get_scan_renderer();

/** \brief Render consecutive concentric rings around station 36.
  * \details Concentric red, green, and blue rings expanding out from the centre of the display.
  *   Every second frame is skipped, so the rings move out at a pace of 12.5 station/pixels per
  *   second.
  * \ingroup display_renderer
  */
const struct renderer_t* get_ring_renderer();

#endif
