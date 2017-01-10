# EEPROM layout {#eeprom_layout}

The display's microcontroller uses its available EEPROM to store some display-specific details
such as the serial number, the number and type of LEDs, etc.
The following table provides an overview of the EEPROM sections currently defined and used by
the display controller firmwares.
The EEPROM sections are all put in a specific code section. Linker flags or a custom linker script
should be used in order to ensure these sections end up in the EEPROM.

Description              | Address offset  | Length             | Code section
-------------------------|----------------:|-------------------:|-------------
Serial number            | 0x000           | 32                 | .serialno
Display LED information  | 0x020           | varies; at most 16 | .displayprop
Display LED map          | 0x030           | varies             | .portmap

Firmwares are allowed to use the reserved EEPROM segments for LED information and LED mapping
as they see suit, provided they stick to the reserved space.
Please refer to the different implementation pages from the \ref display_implementations
"list of existing implementations".


\par Serial number
The serial number is stored as a null-terminated UTF16-LE string, as required by the USB
specification.
The 32 bytes of reserved EEPROM can therefore be used to store 15 characters.
See \ref display_implementations on which format to use for the serial number string.
```{.c}
// Assuming the platform is little-endian
const char16_t USB_STRING_SERIALNO[] __attribute__((section(".serialno")) = u"ICD-IC-000-0000";
```

\par Display LED information
Reserved to store 16 bytes of information on the LED type, RGB color order, LED count...


\par Display LED map
Since the electrical connections between the LEDs is very likely to be different from the
sequential string numbers, space is reserved to store a mapping of buffer offsets to the
physical layout of LEDs.

