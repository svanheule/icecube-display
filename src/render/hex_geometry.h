#ifndef RENDER_HEX_GEOMETRY_H
#define RENDER_HEX_GEOMETRY_H

#include "stdint.h"

/// Calculate the distance between strings \a x and \a y (zero-indexed)
uint8_t get_string_distance(uint8_t x, uint8_t y);

/// Calculate the distance between a strings (zero-indexed) and the centre of the array
/// (string 36). This is equivalent to calling get_string_distance(string, 35).
#define get_string_distance_to_centre(string) get_string_distance((string), 35);

#endif
