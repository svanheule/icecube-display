# USB communication {#usb_communication}

The LED display is equiped with a USB port to provide the possibility of using a PC
to display IceCube event data not embedded in the device's firmware,
or make use of nicer effects that aren't so easy to produce using only the microcontroller.
A full-speed USB 2.0 compliant interface is provided, using `0x1CE3` as a vendor ID.
The vendor ID is *not* registered with the [USB-IF](http://usb.org), as this would be
[rather expensive](http://www.usb.org/developers/vendor/) for a small scale project like this.

The full-speed interface provides a bandwidth of 12Mbps, enough to run thousands of LEDs at 25 FPS.
The display interface is provided in a very generic fashion, so any application that can
render (low resolution) RGB data at video frame rates, can show this on the display.

## USB device details

Aside from the \ref ::usb_request_code_t "standard request" support required by
the USB standard for proper functioning of the device, a number of
\ref ::vendor_request_t "vendor specific requests" can also be handled.

The interface presented by the device consists of two endpoints:
* EP0: default control endpoint for status reporting and control
  * \subpage usb_endpoint_control
  * \subpage display_metadata
* EP1: bulk endpoint for frame data transfer to the device
  * \subpage usb_remote_renderer


Interfacing with the device will typically require querying it for information, and then
configuring the device and software accordingly.
Since the device implements a vendor specific class, only the minimally required setup can be
performed by the host PC.
The \ref usb/endpoint_0.h "default control endpoint" handles these tasks.
* Reading the device and interface descriptors,
* Setting the device address,
* Selecting a configuration, in this case configuration 1.

After enumeration is completed, the device can be used by the user.
Unless it is known beforehand which device will be present, the display has to be queried to check
which LED type it supports, how many LEDs are present, and what kind of data that can be displayed.
Querying the display is achieved through a \ref ::VENDOR_REQUEST_DISPLAY_PROPERTIES
"vendor specific control request" to obtain a \ref display_metadata "display metadata report".
Once the buffer structure and content is determined based on the display properties, a display
frame buffer can be filled with appropriate data and transmitted to the display. This is achieved
by sending the buffer data to the \ref usb_remote_renderer "bulk endpoint".

### Device state management
* \ref usb/device.h "Device state": See §9.1.1 and Figure 9-1 of the USB 2.0 specification
* Device \ref usb/address.h "addressing" and \ref usb/configuration.h "configuration:
  See §9.1.2 of the USB 2.0 specification

### USB endpoint hardware details:
* \ref usb_endpoint_status
* \ref usb_endpoint_config
* \ref usb_platform


## Multi-segment display facilities

It is possible to construct a single large display, e.g. to represent IceCube, consisting of
multiple segments each driven by its own microcontroller.
Since every microcontroller represents a single USB device, these actually behave as independent
displays.
When displaying fast moving images on these composite displays however, care has to be taken to
prevent tearing.
This occurs when one segment is displaying frame data ahead of an adjacent segment, resulting in
visible discontinuities.
To prevent tearing, different segments should display data from the same frame as close together in
time as possible.
Some initial synchronisation mechanism has been provided, described in \subpage led_display_timing.

