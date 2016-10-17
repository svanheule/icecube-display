#ifndef RENDER_TEST_RING_H
#define RENDER_TEST_RING_H

/** \file
  * \brief Simple renderer that runs on the microcontroller to test the display.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include "render/renderer.h"

/** \brief Render consecutive concentric rings around station 36.
  * \details Concentric red, green, and blue rings expanding out from the centre of the display.
  *   Every second frame is skipped, so the rings move out at a pace of 12.5 station/pixels per
  *   second.
  * \ingroup display_renderer
  */
const struct renderer_t* get_ring_renderer();

#endif
