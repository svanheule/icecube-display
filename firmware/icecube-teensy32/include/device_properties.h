#ifndef DEVICE_PROPERPTIES_H
#define DEVICE_PROPERPTIES_H

/** \file
  * \brief Additional platform specific display properties.
  * \details Platform specific functionality that extends the properties provided by
  *   display_properties.h.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include <stdbool.h>

/** \brief Return the initial data direction of a strip segment group.
  * \details The data lines for the strip segment groups alternate between going top-to-bottom and
  *   bottom-to-top. The orientation of the first strip segment determines the orientation of the
  *   following segments.
  *   The original design placed to electronics in the bottom of the display, so reverses the
  *   first LED strip.
  * \return `true` if the first strip is reversed, `false` otherwise. The default value if the
  *   EEPROM is cannot be read or is not initialised is `true`.
  */
bool get_reverse_first_strip_segment();

#endif // DEVICE_PROPERTIES_H
