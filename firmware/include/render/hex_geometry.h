#ifndef RENDER_HEX_GEOMETRY_H
#define RENDER_HEX_GEOMETRY_H

/** \file
  * \brief Hexagonal grid geometry support
  * \author Sander Vanheule (Universiteit Gent)
  */

#include "stdint.h"

/// \name String grid distance
/// @{

/** \brief Calculate the distance between strings \a x and \a y (zero-indexed)
  * \ingroup led_display_renderer
  * \details
  * A hexagonal lattice may be constructed from a regular, 3D square grid by taking the plane
  * satisfying the eqaution \f$x+y+z=0\f$. If the pair \f$(x,y)\f$ is mapped onto a 2D space
  * given by \f$(v,w)\f$, this plane equation determines \f$z\f$.
  * The distance \f$D\f$ between 2 points on a hexagonal grid is given by
  * \f{eqnarray*}{
  *     D &=& 0.5 (|x_1-x_2| + |y_1-y_2| + |z_1-z_2|) \\
  *       &=& 0.5 (|v_1-v_2| + |w_1-w_2| + |(v_1-v_2)-(w_1-w_2)|)
  * \f}
  * \see Extensive explanation at: http://www.redblobgames.com/grids/hexagons/
  */
uint8_t get_string_distance(uint8_t x, uint8_t y);

/// Calculate the distance between a strings (zero-indexed) and the centre of the array
/// (string 36). This is equivalent to calling get_string_distance(string, 35).
/// \ingroup led_display_renderer
#define get_string_distance_to_centre(string) get_string_distance((string), 35);

/// @}

#endif
