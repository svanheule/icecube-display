#ifndef RENDER_TEST_SCAN_H
#define RENDER_TEST_SCAN_H

/** \file
  * \brief Simple renderer that runs on the microcontroller to test the display.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include "render/renderer.h"

/** \brief Render a number of pixels scanning from the start to end of the buffer, and back.
  * \details Generates three white pixels that slide through the buffer, effectively
  *   scanning the display line by line.
  * \ingroup led_display_renderer
  */
const struct renderer_t* get_scan_renderer();

#endif
