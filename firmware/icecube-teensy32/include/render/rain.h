#ifndef RENDER_RAIN_H
#define RENDER_RAIN_H

/** \file
  * \brief Simple renderer that runs on the microcontroller to test the display.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include "render/renderer.h"

/** \brief Render RGB rain.
  * \details Generates groups of a red, a green, and a blue pixel that move down each string.
  *   These groups are separated by a number of off pixels, generating a rain-like effect.
  * \ingroup display_renderer
  */
const struct renderer_t* get_rain_renderer();

#endif
