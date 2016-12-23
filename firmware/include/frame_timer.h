#ifndef FRAME_TIMER_H
#define FRAME_TIMER_H

/** \file
  * \brief Frame draw timing.
  * \details Timer to trigger drawing of a new frame DEVICE_FPS times per second.
  * \author Sander Vanheule (Universiteit Gent)
  */

/** \defgroup led_display_timing Frame timing
  * \ingroup led_display
  * \brief Timer that triggers drawing of a frame 25 times per second.
  * \details When the device is connected to a USB host, it can also correct the timer
  *   interval to correspond to 40ms. To achieve this synchronisation, new_sof_received() should
  *   be called every (few) SOF token(s). The number of clock ticks between these calls will
  *   be monitored and averaged out to slave the 25FPS timer to the USB SOF timer.
  *   Using the correct_display_frame_counter() and correct_display_frame_phase() functions,
  *   all display segments can be made to update within 1ms from each other.
  *   Note that tearing can still occur if no mechanism is present to tell the segments when
  *   the remotely rendered data should be displayed.
  * \todo A USB UVC-like interface should be implemented for displays consisting of multiple
  *   segments to prevent frame tearing.
  */

#include <stdint.h>
#include <stdbool.h>

/// Time interval between frame displays expressed in milliseconds.
#define MS_PER_FRAME (1000/DEVICE_FPS)

/// \name Frame timing tracking
/// @{

/// Function to be called upon receival of a USB SOF token with the new USB frame counter value.
void new_sof_received(const uint16_t usb_frame_counter);

/** \brief Display frame counter and usb frame counter values at frame draw time.
  * \ingroup led_display_timing
  * \details Upon frame timer roll-over, the USB frame counter value is latched such that
  *   a comparison can be made between multiple devices as to when they are drawing frames
  *   relative to each other. If different devices are in phase, then the diffence between the
  *   usb frame counter values should be an exact multiple of 40 (i.e. 40 ms or 25 FPS), with
  *   the factor of 40 being the difference between the display frame counters.
  */
struct display_frame_usb_phase_t {
    uint16_t display_frame_counter;
    uint16_t usb_frame_counter;
};

/** \brief Latest display frame counter phase.
  * \ingroup led_display_timing
  * \details Copy the latest display frame phase into the provided pointer.
  * \return The return value indicates whether the value stored in counter_phase is valid.
  */
bool get_display_frame_usb_phase(struct display_frame_usb_phase_t* usb_phase);

/** \brief Correct the display frame counter.
  * \ingroup led_display_timing
  * \return Whether the frame counter value was actually corrected or not.
  */
bool correct_display_frame_counter(const int16_t frame_diff);

/** \brief Shift the frame display phase with respect to the USB frame counter.
  * \ingroup led_display_timing
  * \details The absolute value of \a ms_shift should be smaller than or equal to
  *   half the number of milliseconds per display frame interval.
  *   The provided ms shift can be performed only once per frame cycle, so if multiple calls
  *   are performed, only the last correction of the cycle will be used.
  */
void correct_display_frame_phase(const int8_t ms_shift);

/// @}


/// \name Frame draw triggering
/// @{

/// Initialise the frame timer.
/// \ingroup led_display_timing
void init_frame_timer();

/// Whether a new frame should be displayed or the device is allowed to idle.
/// \ingroup led_display_timing
bool should_draw_frame();

/// Acknowledge that a frame has been drawn.
/// \ingroup led_display_timing
void clear_draw_frame();

/// @}

#endif //FRAME_TIMER_H
