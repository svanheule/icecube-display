#include "util/tlv_list.h"
#include <avr/pgmspace.h>

uint16_t get_tlv_list_length_P(const struct dp_tlv_item_t* tlv_list) {
  uint16_t total = 0;
  uint8_t field_type = pgm_read_byte(&tlv_list->type);
  while (field_type != TLV_TYPE_END) {
    // Add header and item length to total length
    total += 2 + pgm_read_byte(&tlv_list->length);
    // Proceed to next item
    ++tlv_list;
    field_type = pgm_read_byte(&tlv_list->type);
  }
  return total;
}
