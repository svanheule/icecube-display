IceTop LED event display {#mainpage}
========================

## The IceTop detector ##

The surface detector of the IceCube neutrino observatory is better known as the IceTop detector.
It's purpose is to detect and measure extensive airshowers produced by highly energetic particles
colliding with the earth's atmosphere at the South Pole, by measuring the Cherenkov light the
particles in the air shower produce when they cross the detector.

The IceTop detector consists of 81 stations, each consisting of two tanks.
Each tank has two photosensitive detection devices called DOMs or Digital Optical Modules.
One of these DOMs is configured to detect small signals, the other to correctly detect the large
which would saturate the first DOM.
Of the 81 stations, the 3 in-fill stations are placed in the middle to create a more densely
instrumented region of the detector, allowing it to detect air showers of lower energy.

The picture below shows the geometry of the detector, as well as the 3 in-fill stations in gray.
The LED display mimics this geometry, by putting each station on a regular hexagonal grid, instead
of the slightly irregular grid of the actual detector.
The in-fill stations have not been included in the display due to a lack of space for the LEDs.

\image html icetop-array.png "IceTop detector array layout"
\image latex icetop-array.pdf "IceTop detector array layout" width=0.7\textwidth

## Display usage ##

The display has a number of input buttons and LEDs to interact with and relay status information to
the user.
The green *Power* LED indicates wether the display is receiving 5V power input.
The orange *USB activity* LED will light up when there is a USB connection, and blink when there
is bus activity, e.g. when display data is transmitted.

\image html front.png "Side view of the LED display"
\image latex front.pdf "Side view of the LED display" width=0.9\textwidth

### Stand-alone operation ###
In stand-alone mode, when the display is not connected via USB and only powered on, the display can
also show a number of IceTop events stored on the device itself.
For every event, first a time-lapse will be shown, followed by an overview.
The light intensity of the LEDs is related to the amount of light collected by the corresponding
IceTop station, and the colour is related to the time when this light was collected.
Red means early in the event, blue means late.
After pressing either of the blue buttons, the display will start cycling through the 9 stored
IceTop events.
At any point, you can go to the next event using the *Forward* button. You can also pause an event
overview by pressing *Play/Pause* during playback, or resume playback by pressing the button again
when the display is paused.

First, three down-going events of increasing energy will be shown.
Then come three slightly inclined events, and three events with large inclination, again each group
with increasing energy.
As the energy of the events increases, you will see that the part of the detector that is activated
by the airshower also increases.
Also, as the inclination of the showers increase, it becomes clear that the shower is actually
contained in a moving surface when it hits the detector.
Downgoing events activate the detector almost simultaneously, while inclined events seem move along
the surface of the detector.

### USB connectivity and steamshovel ###

The device's USB port can be used to transfer display data from a USB host. A detailed overiew
can be found in the [firmware documentation](\ref usb_endpoint). A Steamshovel artist has been
implemented using this connectivity to be able to display any simulation or data file.

The options of the LedDisplay artist have been modelled after the Bubbles artist:
  * *Device*: select one of the detected LED displays. Detection happens once at start-up.
  * *Static color* and *brightness*: Select a color and brightness for static DOM maps.
  * *Compression*: Exponent of the power law used to compress hit info. Since a linear scale would
      hide the details in low charges, the high charges are compressed, giving the display a larger
      dynamic range. Note that display gamma correction is performed independently of this setting.
  * *Colormap*: time to color mapping.
  * *Finite pulses*: If 'Use infinite pulses' is checked, a hit will be shown starting from its time
      of detection. Otherwise it will disappear after the selected duration. This is different from
      the Bubbles setting in that the highest value of the pulse duration slider isn't interpreted
      as positive infinity, but just 10⁵ns.

\image html screenshot-steamshovel.png "Steamshovel with LedDisplay artist"
\image latex screenshot-steamshovel.png "Steamshovel with LedDisplay artist" width=0.7\textwidth

## Firmware upgrade ##

The device's firmware can be upgraded either via USB, or by using the ICSP header on the PCB.
For normal upgrades, it is recommended to use USB, since this will only modify the part
of the firmware that handles normal operation, and leaves the bootloader (used for USB firmware
upgrades) and the EEPROM (used to store static information like the serial number) alone.

### USB firmware upgrade ###

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


### ICSP firmware upgrade ###

The device can also be programmed using the ICSP header on the board. To access this header, the
bottom plate of the device needs to be removed. Pin 1 of the header is marked with a slightly offset
angle, and is next to the `P3` marking.
The VCC line of the ICSP header can be used to power the device, so make sure that the
programmer's VCC line is either disconnected, or that the external power suppy is not used!

With an ICSP programmer, it is recommended to use the cmake system to program the fuses and
lock bits, the EEPROM, the program flash, and the bootloader flash. When performing the initial
flash of the device after assembly, a complete chip erase should be performed beforehand to ensure
the lock bits are reset and a new firmware can be flashed to the device.

