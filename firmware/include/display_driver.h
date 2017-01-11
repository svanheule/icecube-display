#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

/** \file
  * \brief Microcontroller specific implementation of the display driver.
  * \details Provides proper initialisation of the microcontroller hardware to drive the
  *   LED communications and write frame data.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include "frame_buffer.h"

/** \defgroup led_display_driver Display driver
  * \ingroup led_display
  * \brief LED communication interface.
  * \details Provides proper initialisation of the hardware required to drive the display LEDs.
  *   After initialisation, frame buffers can be displayed.
  *   Display clearing is supported separately by display_blank() as it may be possible to
  *   provide a more efficient implementation compared to just writing a empty frame buffer.
  * @{
  */

/// \name Display initialisation
/// @{

/** \brief Initialise the hardware required for driving the display.
  * Must be called before display_frame().
  */
void init_display_driver();

/// @}


/// \name Display updating
/// @{

/** \brief Write a frame out to the display from the given frame buffer.
  * \details During the write-out, the ::FRAME_DRAW_IN_PROGRESS flag will be set on the frame.
  * \param buffer Valid point to a frame buffer. Must not be a null-pointer!
  */
void display_frame(struct frame_buffer_t* buffer);

/** \brief Turn all the LEDs off.
  * \details This achieves the same effect as calling display_frame() with an all-zero frame buffer
  *   but is more efficient since it requires less memory and time to run.
  */
void display_blank();

/// @}
/// @}

#endif
