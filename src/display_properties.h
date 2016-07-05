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

/// Order in which the RGB bytes should be pushed out to the display.
enum display_led_color_order_t {
    LED_ORDER_RGB = 0
  , LED_ORDER_BRG = 1
  , LED_ORDER_GBR = 2
  , LED_ORDER_BGR = 3
  , LED_ORDER_RBG = 4
  , LED_ORDER_GRB = 5
};

/// Number of LEDs, which represent IceTop stations, present in the display.
enum display_led_count_t {
    LED_COUNT_IT78 = 78 ///< Stations 1-78, no in-fill stations.
  , LED_COUNT_IT81 = 81 ///< Stations 1-81, includes the in-fill stations.
};

const struct dp_tlv_item_t* get_display_properties_P();
uint16_t get_tlv_list_length_P(const struct dp_tlv_item_t* tlv_data);

void init_display_properties();

uint8_t get_led_count();

/// The order in which the RGB data should be transmitted per LED.
enum display_led_color_order_t get_color_order();

/// Item in a TLV list.
struct dp_tlv_item_t {
  enum display_property_type_t type; ///< Type of display data.
  uint8_t length; ///< Length of this field's data not including the two type and length bytes.
  enum memspace_t memspace; ///< Memory region the TLV data resides in.
  const void* data; ///< Pointer to the memory block contain the TLV data.
};

#endif //DISPLAY_PROPERPTIES_H
