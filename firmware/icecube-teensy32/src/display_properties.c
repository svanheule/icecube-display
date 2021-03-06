#include "display_properties.h"
#include "device_properties.h"
#include "display_types.h"
#include "frame_buffer.h"
#include <avr/eeprom.h>
#include <stdbool.h>

struct dp_information_range_t {
  uint8_t start;
  uint8_t end;
} __attribute__((packed));

static struct dp_information_range_t dp_info_range_icecube;
static const struct dp_information_range_t dp_info_range_deepcore = {79, 86};

static bool has_deepcore;
static uint16_t led_count;

struct dp_led_information_t {
  uint8_t led_type;
  uint8_t color_order;
  uint8_t ic_string_start;
  uint8_t ic_string_end;
  uint8_t has_deepcore;
  uint8_t reverse_first_strip_segment;
} __attribute__((packed));

#define DISPLAYPROP __attribute__((section(".displayprop"), used))
static const struct dp_led_information_t DP_LED_INFORMATION DISPLAYPROP = {
    LED_TYPE_WS2811
  , LED_ORDER_GRB
  , DEVICE_ICECUBE_STRING_START
  , DEVICE_ICECUBE_STRING_END
  , DEVICE_HAS_DEEPCORE
  , DEVICE_REVERSE_FIRST_STRIP_SEGMENT
};

#define GROUPPROP __attribute__((section(".groupid"), used))
static const uint8_t DP_INFO_GROUP[16] GROUPPROP;

static const enum display_led_type_t DP_INFO_LED_TYPE = LED_TYPE_WS2811;
static const enum display_information_type_t DP_INFO_TYPE = INFORMATION_IC_STRING;

static uint16_t dp_buffer_size;

static const struct dp_tlv_item_t PROPERTIES_TLV_LIST[] = {
    TLV_ENTRY(DP_INFORMATION_RANGE, MEMSPACE_PROGMEM, &dp_info_range_deepcore)
  , TLV_ENTRY(DP_LED_TYPE, MEMSPACE_PROGMEM, &DP_INFO_LED_TYPE)
  , TLV_ENTRY(DP_INFORMATION_TYPE, MEMSPACE_PROGMEM, &DP_INFO_TYPE)
  , TLV_ENTRY(DP_BUFFER_SIZE, MEMSPACE_RAM, &dp_buffer_size)
  , TLV_ENTRY(DP_INFORMATION_RANGE, MEMSPACE_RAM, &dp_info_range_icecube)
  , TLV_ENTRY(DP_GROUP_ID, MEMSPACE_PROGMEM, &DP_INFO_GROUP)
  , TLV_END
};

union eeprom_start_t {
  char bytes[4];
  uint32_t dword;
};
static const union eeprom_start_t EEPROM_START = {.bytes = {'I', 0, 'C', 0}};
extern const uint8_t __eeprom_start[];
static bool use_eeprom;

void init_display_properties() {
  // Read actual values from EEPROM
  use_eeprom = EEPROM_START.dword == eeprom_read_dword((uint32_t*) __eeprom_start);

  if (use_eeprom) {
    dp_info_range_icecube.start = eeprom_read_byte(&DP_LED_INFORMATION.ic_string_start);
    dp_info_range_icecube.end = eeprom_read_byte(&DP_LED_INFORMATION.ic_string_end);
    led_count = 60*(dp_info_range_icecube.end-dp_info_range_icecube.start+1);
    has_deepcore = eeprom_read_byte(&DP_LED_INFORMATION.has_deepcore);

    if (has_deepcore) {
      led_count += 60*(dp_info_range_deepcore.end-dp_info_range_deepcore.start+1);
    }

    dp_buffer_size = get_frame_buffer_size();
  }
}

uint16_t get_led_count() {
  return led_count;
}

uint8_t get_led_size() {
  return sizeof(struct led_t);
}

enum display_led_color_order_t get_color_order() {
  if (use_eeprom) {
    return eeprom_read_byte(&DP_LED_INFORMATION.color_order);
  }
  else {
    return 0;
  }
}

bool get_reverse_first_strip_segment() {
  if (use_eeprom) {
    return eeprom_read_byte(&DP_LED_INFORMATION.reverse_first_strip_segment);
  }
  else {
    return true;
  }
}

const struct dp_tlv_item_t* get_display_properties_P() {
  if (has_deepcore) {
    return &(PROPERTIES_TLV_LIST[0]);
  }
  else {
    return &(PROPERTIES_TLV_LIST[1]);
  }
}
