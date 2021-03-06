#ifndef RENDER_DEMO_H
#define RENDER_DEMO_H

/**
 * \file
 * \brief Stand-alone rendering mode for showing simplified IceTop events.
 * \see switches.h
 * \author Sander Vanheule (Universiteit  Gent)
 */

#include "render/renderer.h"

/** \brief Renderer for stand-alone operation cycling through a number of stored IceTop events.
  * \details The events are stored in the microcontroller's flash memory. Every event is first
  *   shown in a time lapse mode, after which an overview of the entire event is shown.
  *   Currently 9 events are included and displayed in the following order:
  *     * Three 'vertical' events (\f$ \cos\theta\in[0,0.1]\f$), displayed in increasing energy,
  *     * Three slight inclination events (\f$\cos\theta \in[0.2,0.25]\f$), displayed in
  *       increasing energy,
  *     * Three inclined events (\f$\cos\theta \in[0.3,0.35]\f$), displayed in increasing energy.
  *
  *   The *Forward* button can be used to cycle through the stored events, and the *Play/Pause*
  *   button can be used to pause display to the current event.
  *   When paused, the display will show the event overview. If the user then presses play again,
  *   the renderer will again start in the time lapse mode for the event currently selected event.
  * \ingroup led_display_renderer
  */
const struct renderer_t* get_demo_renderer();

#endif //RENDER_DEMO_H
