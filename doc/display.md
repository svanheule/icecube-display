IceTop LED event display {#mainpage}
========================

## Usage ##

![Side view of the LED display](doc/pcb-user/front-bg-alpha.png)


## USB communication ##

The LED display is equiped with a USB port to provide the possibility of using a PC to display
IceCube event data not embedded in the device's firmware, or make use of nicer effects that aren't
so easy to produce using only the microcontroller.
A full-speed USB 2.0 compliant interface is provided, using `0x1CE3` as a vendor ID, and `0x0001`
as product ID. The vendor ID is *not* registered with the [USB-IF](http://usb.org), as this would
be [rather expensive](http://www.usb.org/developers/vendor/) for a small scale project like this.
The full-speed interface provides a bandwidth of 12Mbps. To display 25 uncompressed frames per
second, 62.4kbps is required (not including protocol overhead). This interface is provided in a
very generic fashion, so any application can display (low resolution) RGB data at video frame
rates.

### Protocol specifics ###
In a first implementation, the control endpoint required by the USB standard is extended with a
vendor specific command. See section 9.3 of the
[USB 2.0 specification](http://www.usb.org/developers/docs/usb20_docs/) for more details.

Name       | bRequest
:----------| -------:
PUSH_FRAME |        1


#### PUSH_FRAME ####
Pushes a single frame to be shown on the display into the frame buffer queue. When connected via
USB all other renderers are stopped, so this frame will be displayed until the next one is pushed.
Note that the device only updates the display 25 times per second, so pushing frame more
frequently than this will result in buffer overflows on the device and consequently control
transfers will be stalled until memory is freed.

Field         | Value
--------------|------------:
bmRequestType | 0b0_10_00001
bRequest      |            1
wValue        |            0
wIndex        |            0
wLength       |   312 (78×4)


### USB command example ###

A device display buffer consists of 312 bytes, corresping to the 4 bytes needed by every LED.
Data for the APA102 ICs is described by: `(111X:XXXX BBBB:BBBB GGGG:GGGG RRRR:RRRR)`
  1. Five global brightness bits, used for PWMing the RGB subpixels, preceded by three '1' bits,
  2. Three bytes giving the RGB value to be displayed.

The following example pushes two frames to the device:
  * Frame where every LED displays red at half the maximum brightness,
  * Empty frame to clear the display.

~~~{.py}
  import usb1

  REQUEST_PUSH_FRAME = 1

  cxt = usb1.USBContext()
  devs = cxt.getDeviceList(skip_on_error=True)

  ic_dev = None
  for dev in devs:
    if dev.getVendorID() == 0x1CE3:
      ic_dev = dev

  if not ic_dev is None:
    frame = [0]*78*4
    for station in range(78):
      frame[4*station:4*(station+1)] = [0x10, 255, 0, 0]

    handle = ic_dev.open()

    handle.controlWrite(
        usb1.ENDPOINT_OUT | usb1.TYPE_VENDOR | usb1.RECIPIENT_INTERFACE
      , REQUEST_PUSH_FRAME
      , 0
      , 0
      , bytes(frame)
    )

    handle.controlWrite(
        usb1.ENDPOINT_OUT | usb1.TYPE_VENDOR | usb1.RECIPIENT_INTERFACE
      , REQUEST_PUSH_FRAME
      , 0
      , 0
      , bytes([0]*78*4)
    )
   
    handle.close()
~~~


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

