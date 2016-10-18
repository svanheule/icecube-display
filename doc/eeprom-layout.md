## EEPROM layout ##

The Atmega32U4 has 1024 bytes of EEPROM memory and this is preserved when updating the display
firmware. Teensy devices can also be configured to use part of their flash as persistent EEPROM.
This allows us to store some device specific information such as the serial number and LED
configuration.

Description              | Address offset  | Length
-------------------------|----------------:|------:
Serial number            | 0x0000          | 0x20
Display LED information  | 0x0020          | 0x03/0x05
Display string map       | 0x0030          | 0x24

### Serial number ###
The serial number is stored as a null-terminated UTF16-LE string, as required by the USB
specification. The used format is "ICD-XX-YYY-ZZZZ":
* XX: 'IT' or 'IC', depending on whether the device is meant to display IceTop or IceCube.
* YYY: Sequential number indicating the display model. All displays with the same XX-YYY value
    should be able to use the same device firmware.
* ZZZZ: Sequential number to provide a unique identifier for the hardware.

### Display LED information ###
For the IceTop displays (ICD-IT-001-ZZZZ),
3 bytes of EEPROM are used to store display specific information:
* *0x0* LED count: 78 for the small display, 81 for the large display; see ::display_led_count_t
* *0x1* Supported LED type: see ::display_led_type_t
* *0x2* LED RGB order: see ::display_led_color_order_t

For the IceCube displays (ICD-IC-001-ZZZZ), 5 bytes or EEPROM are used.
The first and last supported string number denote a continous, inclusive range of IceCube strings
that can be displayed by the device.
* *0x0* Supported LED type: see ::display_led_type_t
* *0x1* LED RGB order: see ::display_led_color_order_t
* *0x2* First supported IceCube string number (in range 1-78)
* *0x3* Last supported IceCube string number (in range 1-78)
* *0x4* Support for DeepCore strings (boolean)

### String map ###
Currently, only the IceCube displays store a string-to-buffer offset mapping in the EEPROM as there
may be quite some variability in how the connections between LED strips is layed out.
This EEPROM segment contains 4×(1+8) bytes. The first of each of these nine bytes indicates how
many values in the following 8-byte array are valid.

As an example, consider the following EEPROM data for a display which uses all 8 ports for the first
two segments, but only the first 7 ports for the last two segments.
The last bytes of the two last segment mappings are shown here as '`xx`' for "don't care".
They can be any value, but are usually '`00`'. Care should be taken that mappings for non-existant
LED strip segments don't go outside of the supported IceCube string range, but rather just re-use
LED data from other strings if some data has to be transmitted.
The values contained in the segment mapping are always relative to the lowest supported IceCube
string number. If the supported string range for this display is e.g. 1-30, then the first
segment on the first port maps to string 1+7=8.

Offset   | Data (hexadecimal)
:--------|:----------------------------
0x30 + 0 | `08 07 0E 0F 08 0B 12 13 0C`
0x30 + 9 | `08 01 16 17 02 05 1A 1B 14`
0x30 + 18| `07 00 15 18 03 04 19 1C xx`
0x30 + 27| `07 06 0D 10 09 0A 11 1D xx`
