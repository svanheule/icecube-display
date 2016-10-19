\addtogroup usb_endpoint
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
In a first implementation, the [default control endpoint](\ref usb_endpoint_control) is
extended with some vendor specific control commands. See section 9.3 of the
[USB 2.0 specification](http://www.usb.org/developers/docs/usb20_docs/) for more details.

Request name       | bmRequestType | bRequest | wValue | wIndex |    wLength
-------------------|---------------|----------|--------|--------|-----------
PUSH_FRAME         |  0b0_10_00000 |        1 |      0 |      0 |     length
DISPLAY_PROPERTIES |  0b1_10_00000 |        2 |      0 |      0 |    2-65535
EEPROM_WRITE       |  0b0_10_00000 |        3 |      0 | offset |     length
EEPROM_READ        |  0b1_10_00000 |        4 |      0 | offset |     length


#### PUSH_FRAME ####

The *PUSH_FRAME* command pushes a single frame to be shown on the display into the frame
buffer queue. When connected via USB all other renderers are stopped, so this frame will be
displayed until the next one is pushed.
Note that the device only updates the display 25 times per second, so pushing frame more
frequently than this will result in buffer overflows on the device and consequently control
transfers will be stalled until memory is freed. Since some platforms may not support
control transfers of size is limited, it is possible to send a single frame using multiple
*PUSH_FRAME* commands.

A device display buffer of the IceTop display for example consists of 312 or 324 bytes,
corresponding to the 4 bytes needed by every LED.
Data for the APA102 ICs is described by: `(111X:XXXX BBBB:BBBB GGGG:GGGG RRRR:RRRR)`
  1. Five global brightness bits, used for PWMing the RGB subpixels, preceded by three '1' bits,
  2. Three bytes giving the RGB value to be displayed.

#### DISPLAY_PROPERTIES ####

*DISPLAY_PROPERTIES* requests a TLV list of [display metadata](\ref led_display_metadata).
A reply to this request will always consist of at least two bytes, which provide the total length
of the response.
A typical query of this metadata will be done the following way:
  1. Perform an IN transfer of wLength 2. This will return the full length of the TLV data as an
     unsigned 16 bit, little endian integer.
  2. Perform an IN transfer of wLength N, with N being the response of the first request.

#### EEPROM_WRITE and EEPROM_READ ####

To perform the (initial) configuration of the device, one may also use the USB interface to read to
and write from the microcontroller's EEPROM.
These two commands allow access to an EEPROM segment of arbitrary length, starting from any
offset address that is within the size of the EEPROM.
Use the *EEPROM_WRITE* command with care, as writing bad data to the EEPROM may render the device
unusable.

The IceCube string to buffer offset mapping of IceCube display microcontrollers can for example
be read using `wIndex=0x30` and `wLength=36`.


### Communication example ###

The following example reads the display information from all connected devices using pyusb:

\include usb.py

