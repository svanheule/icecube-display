#ifndef DISPLAY_PROPERPTIES_H
#define DISPLAY_PROPERPTIES_H

#include <stdint.h>
#include "memspace.h"

enum display_property_type_t {
    DP_END = 0xFF ///< Last field in TLV list with length 0
  , DP_INFORMATION_TYPE = 1
  , DP_INFORMATION_RANGE = 2
  , DP_LED_TYPE = 3
};

enum display_information_type_t {
    INFORMATION_IT_STATION = 0
  , INFORMATION_IC_STRING = 1
};

enum display_led_type_t {
    LED_TYPE_APA102 = 0
  , LED_TYPE_WS2811 = 1
};

#if defined(__DOXYGEN__)
/// Number of LEDs used in the display. This determines the frame buffer size.
/// \ingroup led_display
#define LED_COUNT
#else
#define LED_COUNT DEVICE_LED_COUNT
#endif
#define LED_COUNT_IT78 78
#define LED_COUNT_IT81 81

struct dp_tlv_item_t {
  enum display_property_type_t type;
  uint8_t length;
  enum memspace_t memspace;
  const void* data;
};

const struct dp_tlv_item_t* get_display_properties_P();
uint16_t get_tlv_list_length_P(const struct dp_tlv_item_t* tlv_data);

void init_display_properties();

const uint8_t* get_led_count();
const uint8_t* get_led_mapping();


#endif //DISPLAY_PROPERPTIES_H
