#include "display_properties.h"
#include <avr/eeprom.h>

const uint8_t DP_TLV_DATA[] = {
    DP_STATION_RANGE, 2, 1, LED_COUNT
  , DP_LED_TYPE, 1, LED_TYPE_APA102
  , DP_END, 0
};

uint16_t get_tlv_length_E(const void* tlv_data) {
  const uint8_t* data = (const uint8_t*) tlv_data;
  enum display_property_t field_type;
  do {
    field_type = eeprom_read_byte(data);
    uint8_t field_length = eeprom_read_byte(data+1);
    if (field_type != 0 && field_type != 0xff) {
      data += 2 + field_length;
    }
  }
  while (field_type != 0 && field_type != 0xff);
  return data - (uint8_t*) tlv_data;
}
