#ifndef DISPLAY_TYPES_H
#define DISPLAY_TYPES_H

/** \file
  * \brief IceTop display specific definitions
  * \author Sander Vanheule (Universiteit Gent)
  */

#include <stdint.h>

/** \brief A 4-tuple of bytes describing the data required by the APA102 LEDs.
  * \details The 24-bit RGB values can be scaled using the brightness field to achieve
  *   a larger dynamic range, e.g. to perform gamma correction.
  * \ingroup led_display
  */
struct led_t {
  uint8_t brightness; ///< Global brightness bits; only 5 LSB are valid.
  uint8_t red; ///< 8 bit red component.
  uint8_t green; ///< 8 bit green component.
  uint8_t blue; ///< 8 bit blue component.
} __attribute__((packed));


/// Number of LEDs, which represent IceTop stations, present in the display.
enum display_led_count_t {
    LED_COUNT_IT78 = 78 ///< Stations 1-78, no in-fill stations.
  , LED_COUNT_IT81 = 81 ///< Stations 1-81, includes the in-fill stations.
};

#endif //DISPLAY_TYPES_H
