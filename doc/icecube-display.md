# IceCube LED event display {#icecube-display}

## The IceCube detector

The IceCube detector consist of two parts: the base configuration, consisting of 78 strings, and
the DeepCore in-fill detector consisting of an additional 8 strings with a different spacing.

\image html geometry/icecube-array.png "IceCube string layout"
\image latex geometry/icecube-array.pdf "IceCube string layout" width=0.7\textwidth

## IceCube detector display

The full IceCube detector was broken up into three display modules to facilitate transport,
each containing two power supplies and a Teensy 3.2 microcontroller board.
Provided there is a way to mechanically support the upper part of the module, each of the modules
can be controlled independently.
* Front module: strings 1-30
* Center module: strings 31-50, optionally contains the DeepCore strings as well (79-86)
* Back module: strings 51-78

\image html icecube-segment-layout/segment-assembly.png "Display segment assembly"

A single module contains roughly one third of the IceCube strings.
At 60 DOMs per string, this equates to up to 1800 RGB LEDs that can draw 60mA at 5V
when set to full brightness.
Therefore two 60A 5V power supplies are used in each module to power the LEDs.
Each strip of 60 LEDs is equipped with a circuit breaker rated to 4A to prevent one short circuit
from taking down half the display segment.
To reduce the number of cables running to the fully assembled display, the front and back modules
can get their USB connection to the PC via the center module.
To achieve this, the center module contains a USB hub whose upstream port is accessible from the
side of the module.
Two downstream USB ports are provided facing the front and back modules.

\image html icecube-segment-layout/segment-connections.png "Display segment connections"
\image latex icecube-segment-layout/segment-connections.pdf "Display segment connections" width=0.7\textwidth

### LED strip layout configuration

The Teensy controllers use an 8-bit port to send serial data to 8 LED strips simultaneously.
To make these output pins available, the Teensy are seated in a
[OctoWS2811 adaptor](https://www.pjrc.com/store/octo28_adaptor.html).
The two RJ45 connectors are then fed to two splitter boards using CAT5e cables two provide 8 strip
ports.

Since 8 ports is smaller can the corresponding number IceCube strings per display segment,
the data lines of up to four LED strips are connected in series, resulting in a strip with 60
to 240 LEDs.
The data lines for the first and third strip run from bottom to top, while the data lines for the
second and fourth strip run from top to bottom.
Connections between the strips are made inside the display segment boxes and do not change between
different assemblies of the display.

The way the LED strips are grouped is not predetermined and depends on the length of the available
connecting cables, location of the microcontroller inside the module's bottom box, and of course
personal preferences of the person doing the cabling.
Therefore, the strip configuration is stored in \ref eeprom_layout "the microcontroller's EEPROM"
such that the same firmware can be used in different displays.

A utility is provided (`teensy/update_string_config.py`) to generate the EEPROM data starting from
a JSON file. The below figure shows an example layout of the center module, with the corresponding
JSON snippet.

\image html  icecube-segment-layout/segment-layout.png "Display segment strip layout"
\image latex icecube-segment-layout/segment-layout.pdf "Display segment strip layout" width=0.7\textwidth

The full JSON file may look as the example provided below. Note that not all segments use
all 8 output ports, and not all ports have 4 strips connected in series.
If ports with less than f strips are present, the available ports should be populated starting with
the longest strips.
In the example below, the 'front' module's first 7 ports have four strips, and the last port only
has two.
The 'center' and 'back' modules respectively have 4 and 7 ports used for four strips, and none
on the last port(s).

\include display_config.json
