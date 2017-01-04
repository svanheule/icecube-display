IceCube LED event display {#icecube-display}
=========================

## The deep-ice IceCube detector ##

\todo Write short description of detector

## IceCube display segments ##

The large IceCube display consists of several modules that can be controlled independently
from each other, provided there is a way to mechanically support the upper part of the module.
A single module contains roughly one third (20-30) of the IceCube strings.
At 60 DOMs per string, this equates to up to 1800 RGB LEDs that can draw 60mA at 5V
when set to full brightness.
Therefore two 60A 5V power supplies are used in each module to power the LEDs.
Each strip of 60 LEDs is equipped with a circuit breaker rated to 4A to prevent one short circuit
from taking down half the display segment.

The full IceCube detector was broken up into three parts, each containing two power supplies
and a Teensy 3.2 microcontroller board.
* Front: strings 1-30
* Center: strings 31-50, optionally contains the DeepCore strings as well (79-86)
* Back: strings 51-78

To reduce the number of cables running to the fully assembled display, the front and back modules
can get their USB connection to the PC via the center module.
To achieve this, the center module contains a USB hub whose upstream port is accessible from the
side of the module.
Two downstream USB ports are provided facing the front and back modules.

### LED strip layout configuration ###

\include display_config.json

\image html  segment-layout.png "Display segment strip layout"
\image latex segment-layout.pdf "Display segment strip layout" width=0.7\textwidth
