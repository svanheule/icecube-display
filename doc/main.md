IceCube event data LED display
==============================

The IceCube Neutrino Observatory is a high energy neutrino and cosmic ray detector located at the
Amundsen-Scott South Pole Station.
Its main components -- IceCube and DeepCore in the deep-ice, and IceTop at the glacier's surface --
consist of thousands of Digital Optical Modules or DOMs that detect Cherenkov light emitted by
particles travelling through the ice.
To be able to represent the information about the captured light, a number of LED displays have
been developed over time.
These displays use an (RGB) LED to represent a single DOM or a cluster of DOMs.
Detection time is usually mapped to color, and the amount of light detected is mapped to brightness.
This documentation accompanies the firmware that has been written to drive the displays built
at Ghent University.


## Display electronics ##

To simplify the LED driving, the displays use RGB LED packages that include a driver and serial
communications such as the WS2812B or APA102.
These are then driven in turn by a microcontroller that also sports a USB port to provide
connectivity to a PC.
By allowing the PC to send frames of raw RGB LED data, these displays are essentially a generic
video display, albeit one arranged in a rather odd layout that matches the IceCube detector.

Two displays were built at UGent, sharing much of the firmware code:
* [IceTop LED display](\ref icetop-display)
* [IceCube LED display](\ref icecube-display)


## USB connectivity and steamshovel ##

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
      as positive infinity, but just \f$10^5\f$ns.

\image html screenshot-steamshovel.png "Steamshovel with LedDisplay artist"
\image latex screenshot-steamshovel.png "Steamshovel with LedDisplay artist" width=0.7\textwidth

