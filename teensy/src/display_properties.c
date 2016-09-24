#include "display_properties.h"
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdbool.h>

uint16_t get_tlv_list_length_P(const struct dp_tlv_item_t* tlv_list) {
  uint16_t total = 0;
  enum display_property_type_t field_type = tlv_list->type;
  while (field_type != DP_END) {
    // Add header and item length to total length
    total += 2 + tlv_list->length;
    // Proceed to next item
    ++tlv_list;
    field_type = tlv_list->type;
  }
  return total;
}

struct dp_information_range_t {
  uint8_t start;
  uint8_t end;
} __attribute__((packed));
static struct dp_information_range_t dp_info_range_icecube;
static const struct dp_information_range_t dp_info_range_deepcore = {79, 86};
static uint16_t led_count;

struct dp_led_information_t {
  uint8_t led_type;
  uint8_t color_order;
  uint8_t ic_string_start;
  uint8_t ic_string_end;
  uint8_t has_deepcore;
} __attribute__((packed));

#define DISPLAYPROP __attribute__((section(".displayprop")))
static const struct dp_led_information_t DP_LED_INFORMATION DISPLAYPROP = {
    LED_TYPE_WS2811
  , LED_ORDER_GRB
  , DEVICE_ICECUBE_STRING_START
  , DEVICE_ICECUBE_STRING_END
  , DEVICE_HAS_DEEPCORE
};

static const enum display_information_type_t DP_INFO_TYPE = INFORMATION_IC_STRING;

#define TLV_ENTRY(type, memspace, address) {type, sizeof(*address), memspace, address}
#define TLV_END {DP_END, 0, MEMSPACE_NONE, 0}

static struct dp_tlv_item_t PROPERTIES_TLV_LIST[5] = {
    TLV_ENTRY(DP_LED_TYPE, MEMSPACE_EEPROM, &DP_LED_INFORMATION.led_type)
  , TLV_ENTRY(DP_INFORMATION_TYPE, MEMSPACE_PROGMEM, &DP_INFO_TYPE)
  , TLV_ENTRY(DP_INFORMATION_RANGE, MEMSPACE_RAM, &dp_info_range_icecube)
  , TLV_END
  , TLV_END
};

void init_display_properties() {
  // Initialise Teensy's FlexNVM
/*  eeprom_initialize();*/

  //TODO Read actual values from EEPROM
/*  dp_info_range_icecube.start = eeprom_read_byte(&DP_LED_INFORMATION.ic_string_start);*/
/*  dp_info_range_icecube.end = eeprom_read_byte(&DP_LED_INFORMATION.ic_string_end);*/
  dp_info_range_icecube.start = DEVICE_ICECUBE_STRING_START;
  dp_info_range_icecube.end = DEVICE_ICECUBE_STRING_END;
  led_count = 60*(dp_info_range_icecube.end-dp_info_range_icecube.start+1);

/*  if (eeprom_read_byte(&DP_LED_INFORMATION.has_deepcore)) {*/
  if (false) {
    led_count += 60*(dp_info_range_deepcore.end-dp_info_range_deepcore.start);
    PROPERTIES_TLV_LIST[3] = (struct dp_tlv_item_t)
      TLV_ENTRY(DP_INFORMATION_RANGE, MEMSPACE_PROGMEM, &dp_info_range_deepcore);
  }
}

uint16_t get_led_count() {
  return led_count;
}

enum display_led_color_order_t get_color_order() {
/*  TODO return eeprom_read_byte(&DP_LED_INFORMATION.color_order);*/
  return LED_ORDER_GRB;
}

const struct dp_tlv_item_t* get_display_properties_P() {
  return &(PROPERTIES_TLV_LIST[0]);
}
