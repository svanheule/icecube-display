# Firmware upgrade ##

The device's firmware can be upgraded either via USB, or by using the ICSP header on the PCB.
For normal upgrades, it is recommended to use USB, since this will only modify the part
of the firmware that handles normal operation, and leaves the bootloader (used for USB firmware
upgrades) and the EEPROM (used to store static information like the serial number) alone.

## USB firmware upgrade ##

To perform an upgrade via USB, first power the device using the 5V power supply, and connect the
display to the computer used to perform the upgrade via a USB cable. You should see the green
and orange LED light up.
The display now needs to be rebooted into firmware upgrade mode.
To do this, press and hold the *Forward* button while pressing the *reset* button. After releasing
*reset*, the *Forward* button can also be released and the display should now be in
firmware upgrade mode. The green LED will still be on, but the orange LED will now be off.

When performing the `lsusb` command on Linux, you will now see the Atmel bootloader showing up
in the listing:

    $ lsusb | grep -e "Atmel"
    Bus 001 Device 039: ID 03eb:2ff4 Atmel Corp. atmega32u4 DFU bootloader

When building the firmware from source, you can use the CMake build system to upload the firmware
using the upload target:

    $ cd $BUILD_DIR
    $ cmake $SOURCE_DIR
    $ make upload

If the firmware was provided as a pre-compiled binary file (e.g. as `icetop_display.hex`), you can
upload the firmware using the `avrdude` command:

    # When only one device is connected
    $ avrdude -p ATMEGA32U4 -c flip1 -U flash:w:icetop_display.hex
    # When multiple devices are connected (and ready to be programmed),
    # you should also specify which port number to use (use lsusb to find out)
    $ avrdude -p ATMEGA32U4 -c flip1 -P usb:001:039 -U flash:w:icetop_display.hex


## ICSP firmware upgrade ##

The device can also be programmed using the ICSP header on the board.
Note that any normal firmware upgrade can be performed via USB, so using the ICSP port is not
the recommended way of upgrading your display!
To access the ICSP header, the bottom plate of the device needs to be removed.
Pin 1 of the header is marked with a slightly offset angle, and is next to the `P3` marking.
The VCC line of the ICSP header can be used to power the device, so make sure that the
programmer's VCC line is either disconnected, or that the external power suppy is not used!

With an ICSP programmer, the flash and EEPROM will be erased prior to the device upgrade.
It is recommended to configure the cmake system to generate the right EEPROM data and program
the fuses and lock bits, the EEPROM, the program flash, and the bootloader flash.
When performing the initial flash of the device after assembly, a complete chip erase should be
performed beforehand to ensure the lock bits are reset and a new firmware can be flashed to the
device.
