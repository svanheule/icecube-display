## EEPROM layout ##

The Atmega32U4 has 1024 bytes of EEPROM memory and this is preserved when updating the display
firmware.
This allows us to store some device specific information such as the serial number and LED
configuration.

Description              | Address offset  | Length
-------------------------|----------------:|------:
Serial number            | 0x0000          | 0x20
Display LED count        | 0x0020          | 0x01
Display LED type         | 0x0021          | 0x01
LED data RGB order       | 0x0022          | 0x01

### Display LED properties ###
The following properties are stored on a per-device basis:
* LED count: 78 for the small display, 81 for the large display; see ::display_led_count_t
* Supported LED type: see ::display_led_type_t
* LED RGB order: see ::display_led_color_order_t
