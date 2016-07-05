#include "display_properties.h"
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

uint16_t get_tlv_list_length_P(const struct dp_tlv_item_t* tlv_list) {
  uint16_t total = 0;
  enum display_property_type_t field_type = pgm_read_byte(&tlv_list->type);
  while (field_type != DP_END) {
    // Add header and item length to total length
    total += 2 + pgm_read_byte(&tlv_list->length);
    // Proceed to next item
    ++tlv_list;
    field_type = pgm_read_byte(&tlv_list->type);
  }
  return total;
}

struct dp_information_range_t {
  uint8_t start;
  uint8_t end;
};
static struct dp_information_range_t dp_info_range = {1, 0};

struct dp_led_information_t {
  uint8_t count;
  uint8_t type;
  uint8_t color_order;
};
#define DISPLAYPROP __attribute__((section(".displayprop")))
static const struct dp_led_information_t DP_LED_INFORMATION DISPLAYPROP = {
    DEVICE_LED_COUNT
  , LED_TYPE_APA102
  , LED_ORDER_BGR
};

void init_display_properties() {
  // Read actual value from EEPROM
  dp_info_range.end = eeprom_read_byte(&DP_LED_INFORMATION.count);
}

uint8_t get_led_count() {
  return dp_info_range.end;
}

enum display_led_color_order_t get_color_order() {
  return eeprom_read_byte(&DP_LED_INFORMATION.color_order);
}

static const enum display_information_type_t DP_INFO_TYPE PROGMEM = INFORMATION_IT_STATION;

#define TLV_ENTRY(type, memspace, address) {type, sizeof(*address), memspace, address}
#define TLV_END {DP_END, 0, MEMSPACE_NONE, 0}

static const struct dp_tlv_item_t PROPERTIES_TLV_LIST[] PROGMEM = {
    TLV_ENTRY(DP_LED_TYPE, MEMSPACE_EEPROM, &DP_LED_INFORMATION.type)
  , TLV_ENTRY(DP_INFORMATION_TYPE, MEMSPACE_PROGMEM, &DP_INFO_TYPE)
  , TLV_ENTRY(DP_INFORMATION_RANGE, MEMSPACE_RAM, &dp_info_range)
  , TLV_END
};

const struct dp_tlv_item_t* get_display_properties_P() {
  return &(PROPERTIES_TLV_LIST[0]);
}
