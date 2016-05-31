#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

/** \file
  * \brief Microcontroller specific implementation of the display driver.
  * \details Since the display uses a number of APA102 LEDs connected in series, a hardware SPI
  *   master port can be used to drive the LED chain. On an ATmega microcontroller, this can be
  *   either a hardware SPI port, or a USART port configured as SPI master. The latter case allows
  *   for lower transmission times as it has a hardware buffer that can be kept filled during
  *   transmission. When using a SPI port, the buffer is only the size of the currently transmitted
  *   byte, so one has to wait (and check) until the byte is transmitted to load the next byte,
  *   introducing a small delay between bytes.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include "frame_buffer.h"

/** \brief Initialise the hardware equired for driving the display.
  * Must be called before display_frame().
  */
void init_display_driver();

/** \brief Write a frame out to the display from the given frame buffer.
  * \details During the write-out, the ::FRAME_DRAW_IN_PROGRESS flag will be set on the frame.
  * This function should not be called with a null-pointer for \a buffer!
  */
void display_frame(struct frame_buffer_t* buffer);

/** \brief Turn all the LEDs off.
  * \details This achieves the same effect as calling display_frame() with an all-zero frame buffer
  *   but is more efficient since it requires less time to run.
  */
void display_blank();

#endif
