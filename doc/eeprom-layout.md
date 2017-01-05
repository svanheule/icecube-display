# EEPROM layout {#eeprom_layout}

The display's microcontroller uses its available EEPROM to store some display-specific details
such as the serial number, the number and type of LEDs, etc.
The following table provides an overview of the EEPROM sections currently defined and used by
the display controller firmwares.

Description              | Address offset  | Length
-------------------------|----------------:|------:
Serial number            | 0x000           | 32
Display LED information  | 0x020           | varies; at most 16
Display string map       | 0x030           | varies


## Serial number
The serial number is stored as a null-terminated UTF16-LE string, as required by the USB
specification. The used format is "ICD-XX-YYY-ZZZZ":
* XX: 'IT' or 'IC', depending on whether the device is meant to display IceTop or IceCube.
* YYY: Sequential number indicating the display model. All displays with the same XX-YYY value
    should be able to use the same device firmware.
* ZZZZ: Sequential number to provide a unique identifier for the hardware.


## Display LED information
For the IceTop displays (ICD-IT-001-ZZZZ),
3 bytes of EEPROM are used to store display specific information:

Address offset | Description
--------------:|-------------
0x20 + 0x0     | LED count: 78 for the small display, 81 for the large display
0x20 + 0x1     | Supported LED type: see ::display_led_type_t
0x20 + 0x2     | LED RGB order: see ::display_led_color_order_t


For the IceCube displays (ICD-IC-001-ZZZZ), 5 bytes of EEPROM are used.
The first and last supported string number denote a continous, inclusive range of IceCube strings
that can be displayed by the device.

Address offset | Description
--------------:|-------------
0x20 + 0x0     | Supported LED type: see ::display_led_type_t
0x20 + 0x1     | LED RGB order: see ::display_led_color_order_t
0x20 + 0x2     | First supported IceCube string number (in range 1-78)
0x20 + 0x3     | Last supported IceCube string number (in range 1-78)
0x20 + 0x4     | Support for DeepCore strings (boolean)


## String/strip segment map
Currently, only the IceCube displays store a string-to-buffer offset mapping in the EEPROM as there
may be quite some variability in how the connections between LED strips is layed out.
If only one IceCube string range is supported, these are equal to the string number, minus the
first IceCube string number. If multiple string ranges are supported, these are concatenated
into one continuous mapping.
For example, a display which supports strings 31-50 and 79-86 will have buffer offsets 0-19
correspond to the first string range. Offsets 20-27 then correspond to strings 79-86.

The EEPROM segment contains \f$4\times(1+8)\f$ bytes. The first of each of these nine bytes
indicates how many values in the following 8-byte array are valid.
In the example below shows an EEPROM segment fo a display where only 5 ports are used, but
all ports have 4 segments connected.
The three last bytes (unused) of the segment mappings are shown here as '`00`',
but can be any value.
Care should be taken that mappings for non-existant LED strip segments don't go outside of the
supported IceCube string count, but rather just re-use LED data from other strings if some data
has to be transmitted.

If the EEPROM segment below corresponds to a display with supported string range 31-50, as shown
in the picture, then the first LED strip segment on the first port maps to string 34=31+0x03.
Note that the EEPROM segment is essentially a transpose of the table provided in the configuration
file segment.

<table>
  <caption>Display segment strip buffer offset map</caption>
  <tr>
    <th rowspan="2">Segment</th>
    <th rowspan="2">Address offset</th>
    <th>Ports used</th>
    <th>Buffer mapping</th>
  </tr>
  <tr><th colspan="2">Data (hexadecimal)</th></tr>
  <tr>
    <td>1</td><td>0x30 +  0</td><td><kbd>05</kbd></td><td><kbd>03 0D 04 06 10 00 00 00</kbd></td>
  </tr>
  <tr>
    <td>2</td><td>0x30 +  9</td><td><kbd>05</kbd></td><td><kbd>02 0C 0E 07 11 00 00 00</kbd></td>
  </tr>
  <tr>
    <td>3</td><td>0x30 + 18</td><td><kbd>05</kbd></td><td><kbd>01 0B 0F 08 12 00 00 00</kbd></td>
  </tr>
  <tr>
    <td>4</td><td>0x30 + 27</td><td><kbd>05</kbd></td><td><kbd>00 0A 05 09 13 00 00 00</kbd></td>
  </tr>
</table>
