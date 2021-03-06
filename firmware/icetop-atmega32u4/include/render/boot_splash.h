#ifndef RENDER_BOOT_SPLASH_H
#define RENDER_BOOT_SPLASH_H

/** \file
  * \brief Boot splash renderer.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include "render/renderer.h"

/** \brief Render a simplified version of the 'UGent aula' logo.
  * \details To ensure everybody remembers who developed this display, an adapted version of
  *   the UGent logo can be shown when the device boots. The aula logo normally contains six
  *   pilars, but due to space/resolution constraints, this has been reduced to four.
  *   I guess the university wouldn't aprove of this 'simplification', but this is the
  *   best I could do!
  * \copyright UGent logo is owned by Universiteit Gent
  * \see [Logo Universiteit Gent](http://www.ugent.be/nl/univgent/logo) (Dutch)
  * \ingroup led_display_renderer
  */
const struct renderer_t* get_boot_splash_renderer();

#endif //RENDER_BOOT_SPLASH_H
