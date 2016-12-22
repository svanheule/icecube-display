#ifndef DISPLAY_TYPES_H
#define DISPLAY_TYPES_H

/** \file
  * \brief IceCube display specific definitions
  * \author Sander Vanheule (Universiteit Gent)
  */

#include <stdint.h>

/** \brief A 3-tuple of bytes describing the data required by the WS2811-compatible LEDs.
  * \ingroup led_display
  */
struct led_t {
  uint8_t red; ///< 8 bit red component.
  uint8_t green; ///< 8 bit green component.
  uint8_t blue; ///< 8 bit blue component.
} __attribute__((packed));

#endif //DISPLAY_TYPES_H
