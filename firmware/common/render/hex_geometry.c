#include "render/hex_geometry.h"
#include "avr/pgmspace.h"
#include <stdlib.h>

struct string_coordinates_t {
  int8_t v;
  int8_t w;
};

static const struct string_coordinates_t STRING_VECTOR[] PROGMEM = {
    {0,0}, {1,0}, {2,0}, {3,0}, {4,0}, {5,0}
  , {0,1}, {1,1}, {2,1}, {3,1}, {4,1}, {5,1}, {6,1}
  , {0,2}, {1,2}, {2,2}, {3,2}, {4,2}, {5,2}, {6,2}, {7,2}
  , {0,3}, {1,3}, {2,3}, {3,3}, {4,3}, {5,3}, {6,3}, {7,3}, {8,3}
  , {0,4}, {1,4}, {2,4}, {3,4}, {4,4}, {5,4}, {6,4}, {7,4}, {8,4}, {9,4}
  , {1,5}, {2,5}, {3,5}, {4,5}, {5,5}, {6,5}, {7,5}, {8,5}, {9,5}, {10,5}
  , {2,6}, {3,6}, {4,6}, {5,6}, {6,6}, {7,6}, {8,6}, {9,6}, {10,6}
  , {3,7}, {4,7}, {5,7}, {6,7}, {7,7}, {8,7}, {9,7}, {10,7}
  , {4,8}, {5,8}, {6,8}, {7,8}, {8,8}, {9,8}, {10,8}
  , {5,9}, {6,9}, {7,9}, {8,9}
};

#define STRING_COUNT (sizeof(STRING_VECTOR)/sizeof(STRING_VECTOR[0]))

/* Extensive explanation at: [http://www.redblobgames.com/grids/hexagons/]
 * A hexagonal lattice may be constructed from a regular, 3D square grid by taking the plane
 * satisfying the eqaution \f$x+y+z=0\f$. If the pair \f$(x,y)\f$ is mapped onto a 2D space
 * given by \f$(v,w)\f$, this plane equation determines \f$z\f$.
 * The distance between 2 points on a hexagonal grid is given by
 * \f$0.5 (|x_1-x_2| + |y_1-y_2| + |z_1-z_2|)\f$ or
 * \f$0.5 (|v_1-v_2| + |w_1-w_2| + |(v_1-v_2)-(w_1-w_2)|)\f$.
 */
uint8_t get_string_distance(uint8_t x, uint8_t y) {
  if (x < STRING_COUNT && y < STRING_COUNT) {
    int8_t dv = pgm_read_byte(&STRING_VECTOR[x].v) - pgm_read_byte(&STRING_VECTOR[y].v);
    int8_t dw = pgm_read_byte(&STRING_VECTOR[x].w) - pgm_read_byte(&STRING_VECTOR[y].w);
    return (abs(dv) + abs(dw) + abs(dv-dw))>>1;
  }
  else {
    return 0;
  }
}
