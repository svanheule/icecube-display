#ifndef DISPLAY_PROPERPTIES_H
#define DISPLAY_PROPERPTIES_H

#include <stdint.h>

enum display_property_t {
    DP_END = 0xFF ///< Last field in TLV list with length 0
  , DP_STATION_RANGE = 1
  , DP_STRING_RANGE = 2
  , DP_LED_TYPE = 3
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

extern const uint8_t DP_TLV_DATA[] __attribute__((section(".displayprop")));

uint16_t get_tlv_length_E(const void* tlv_data);

#endif //DISPLAY_PROPERPTIES_H
