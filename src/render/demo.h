#ifndef RENDER_DEMO_H
#define RENDER_DEMO_H

/**
 * \file
 * Renderer for stand-alone operation.
 * Display a number of events stored in the microcontroller's flash memory. Every event is first
 * shown in a time lapse mode, after which an overview of the entire event is shown.
 * Currently 9 events are icluded and displayed in the following order:
 *   * Three 'vertical' events (cosine zenith \f$\in[0,0.1]\f$), displayed in increasing energy
 *   * Three slight inclination events (cosine zenith \f$\in[0.2,0.25]\f$), displayed in increasing
 *     energy
 *   * Three inclined events (cosine zenith \f$\in[0.3,0.35]\f$), displayed in increasing energy
 *
 * \author Sander Vanheule (Universiteit  Gent)
 */

#include <stdint.h>
#include "frame_buffer.h"

// Constants in number of frames (25 fps)
//! On time of an LED for each pulse.
#define PULSE_DURATION 25
//! Off time of the entire display after all pulses.
#define PULSE_CLEAR_DURATION 33
//! Duration of the event overview.
#define OVERVIEW_DURATION 75
//! Off time of the entire display after the event overview.
#define OVERVIEW_CLEAR_DURATION 50

const struct renderer_t* get_demo_renderer();

#endif //RENDER_DEMO_H
