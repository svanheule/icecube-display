#ifndef DISPLAY_PROPERPTIES_H
#define DISPLAY_PROPERPTIES_H

/** \file
  * \brief Device specific display properties.
  * \details As multiple LED configurations are supported, some definitions and functions need
  *   to be provided in order for the firmware to determine how to drive the display.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include <stdint.h>
#include "memspace.h"

/** \defgroup led_display_metadata Display metadata reports
  * \ingroup led_display
  * \brief An extensible interface to provide metadata related to the LED display.
  * \details Display metadata can be requested [over USB](\ref usb_endpoint) as a
  *   type-length-value list. This is an easily extensible interface that can provide additional
  *   information if future firmwares may require so.
  *   An application like Steamshovel can use these metadata reports to automatically configure
  *   which data to send to which display, using the correct buffer format.
  *   With these reports different displays types can be supported without having to hard-code the
  *   specific configuration of each existing device in the application.
  *
  *   ### IceTop display
  *   A typical pocket-icetop TLV list looks as follows:
  *   * ::DP_LED_TYPE (length 1): ::LED_TYPE_APA102
  *   * ::DP_INFORMATION_TYPE (length 1): ::INFORMATION_IT_STATION
  *   * ::DP_INFORMATION_RANGE (length 2): {1, 78}
  *
  *   This implies that LED display consist of APA102 LEDs, which means 4 bytes of information
  *   should be provided per LED.
  *   The information that is displayed corresponds to IceTop stations, so the pulses from
  *   DOMs 61-64 should be merged to determine the LED's color and brightness.
  *   The (inclusive) range of supported IceTop stations is 1-78, so a full display frame consists
  *   of 4×78=312 bytes.
  *
  *   ### IceCube display
  *   The design of the 2m×2m×2m IceCube display consists of three modules to facilitate transport.
  *   If the display controllers were to be ported to the same protocol as the IceTop display, an
  *   example report might look as follows:
  *   * ::DP_LED_TYPE (length 1): ::LED_TYPE_WS2811
  *   * ::DP_INFORMATION_TYPE (length 1): ::INFORMATION_IC_STRING
  *   * ::DP_INFORMATION_RANGE (length 2): {31, 50}
  *   * ::DP_INFORMATION_RANGE (length 2): {79, 86}
  *
  *   This report describes the central part of the display/detector. A frame for this display
  *   contains 3×60×(20+8)=5040 bytes. This is a lot more data than the IceTop display, but even
  *   when using only control transfers, this is still less than 25% of the maximum capacity of a
  *   full speed USB port.
  *   When using an isochronous or bulk endpoint, more bandwidth is available which increases the
  *   headroom on the USB port.
  *
  * @{
  */

/** \brief Different TLV types
  */
enum display_property_type_t {
  /// Last field in TLV list with length 0. Same value as unprogrammed flash/EEPROM to lower
  /// chances of get_tlv_list_length_P() failing when it is provided with a bad pointer.
  DP_END = 0xFF,
  /// Information type, always length 1. See ::display_information_type_t
  /// Allowed only once per metadata report.
  DP_INFORMATION_TYPE = 1,
  /// Information range, always length 2: `{start, end}`.
  /// If multiple ranges are supplies in a single report, the buffer data shoul be concatenated
  /// for these ranges to form a single display buffer.
  DP_INFORMATION_RANGE = 2,
  /// Type of LED used in the display, always length 1. See ::display_led_type_t.
  /// Allowed only once per metadata report.
  DP_LED_TYPE = 3,
  DP_BUFFER_SIZE = 4
};

/** \brief Type of information the display is capable of showing.
  * \details The display can be capable of showing either IceTop tanks (INFORMATION_IT_STATION)
  *   or IceCube strings (INFORMATION_IC_STRING).
  *   If it displays IceTop tanks, then the frame buffer should contain the required LED data
  *   per station, in increasing order as determined by the DP_INFORMATION_RANGE field of the
  *   display metadata: e.g. `{[station 1], [station 2],..., [station 78]}`.
  *   If the display shows IceCube strings, then the frame buffer should contain all DOMs per
  *   string: e.g. `{[dom 1 of string 1], [dom 2 of string 1],..., [dom 60 of string 86]}`.
  */
enum display_information_type_t {
    INFORMATION_IT_STATION = 0 ///< IceTop stations
  , INFORMATION_IC_STRING = 1 ///< Full IceCube strings
};

/// Type of LED IC used in the display.
enum display_led_type_t {
  /// APA102 compatible. Data for each LED consists of 4 bytes: brightness + RGB.
  LED_TYPE_APA102 = 0,
  /// WS2811/WS2812 compatible. Data for each LED consists of 3 bytes: RGB.
  LED_TYPE_WS2811 = 1
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

/// @}

/** \brief Initialise the display properties cache.
  * \details Some display properties are stored in EEPROM which results in slow responses if this
  *   information is requested frequently. Information like the display's number of LEDs is
  *   therefore cached in RAM by this function.
  */
void init_display_properties();

/** \brief Return the (cached) number of LEDs present in the display.
  * \details This function will return 0 before the cache is initialised, or 255 if the EEPROM has
  *   been left unprogrammed. If this function returns a value different from 78 or 81,
  *   you may need to reprogram the EEPROM.
  * \return The number of LEDs present in the display, or 0 if init_display_properties() has not
  *   been called yet.
  */
uint16_t get_led_count();

/// Return the number of bytes required to store the display information for a single LED.
/// \see ::display_led_type_t
uint8_t get_led_size();

/// The order in which the RGB data should be transmitted per LED.
enum display_led_color_order_t get_color_order();

/// Item in a TLV list.
struct dp_tlv_item_t {
  enum display_property_type_t type; ///< Type of display data.
  uint8_t length; ///< Length of this field's data not including the two type and length bytes.
  enum memspace_t memspace; ///< Memory region the TLV data resides in.
  const void* data; ///< Pointer to the memory block contain the TLV data.
};

/** \brief Get a pointer to the TLV list stored in flash.
  * \details The list ends with a ::DP_END field to ensure proper functioning of
  *   get_tlv_list_length_P().
  */
const struct dp_tlv_item_t* get_display_properties_P();

/// Calculate the total length of the TLV list, not including a ::DP_END field.
uint16_t get_tlv_list_length_P(const struct dp_tlv_item_t* tlv_data);

#endif //DISPLAY_PROPERPTIES_H
