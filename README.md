# IceCube LED display
This repository contains design documents and firmware source code for a number of LED displays
designed to show IceCube event data.

## Integration in Steamshovel
A Steamshovel artist was written using pyusb to enable remote rendering.
This allows people already familiar with the IceCube offline event viewer to also be quickly
able to get a new LED display up and running.

## Firmware
The display firmware is written using bare-metal C to provide close integration with the hardware.
Writing a new firmware implementation can take up quite some time, so try to re-use an existing
design if possible. All source code is contained in the `firmware` directory.

Source code documentation containing guidelines for new display and a short description of
existing displays can be compiled using Doxygen:
```{.sh}
$ doxygen Doxyfile
```

A copy of the documentation may be found at
http://icecube.wisc.edu/~svanheule/icecube-display

## Hardware
The `hardware` directory contains design documents for the displays.
This includes both mechanical documents for the display construction, as well as possible
PCB design documents.
