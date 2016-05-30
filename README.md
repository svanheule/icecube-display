# (Almost) Pocket-size IceTop LED display

An LED display consisting of 78 LEDs aranged in a hexagonal shape
to represent the IceTop detector array, with one RGB LED per tank.
Due to the small 16mm spacing of the LEDs, the total display has a size of
about 20cm×24cm.


## Electronics

The LED strips contain APA-102C devices which were
selected due to their adjustable brightness setting and clocked data input
lines. This results in less stringent timing contraints for the controlling
electronics, as the controller can decide how fast the data is clocked, and
clock jitter is less of an issue.

The display is controlled by a custom board, designed around the ATmega32U4 microcontroller.
This microcontroller is similar to the one found on an Arduino Uno, which was used for the initial
prototyping, but additionally has a hardware USB controller
The firmware for the microcontrollers is written in standard C.

### Cost estimate

*Total:* €110 (excl. VAT)
  * 78 APA-102C LEDs (60 LEDs/m strips): €27 (€21/m) ([lednexus](http://lednexus.de))
  * 5V, 5A power supply (HNP40EU-050/HNP36-050): €15 ([schukat](http://schukat.com))
  * Custom PCB: €15 (ordered 5pc, got 6pc) ([eurocircuits](http://eurocircuits.com))
  * Electronic components: €20 ([Conrad](http://conrad.be))
  * Laser cut PMMA (3 plates): €30 ([RU58](http://ru58.nl))


## Microcontroller software design

The framerate for the display will need to be at least 25fps, but can in
principle be higher. The APA-102C LEDs can be clocked at up to 32MHz, so that
a new frame can typically be written out in less 0.2ms. This should leave plenty of
time for the microcontroller to do other things.

The display frames are stored in a queue. A local or remote renderer can push frames in this
queue which will then be popped 25 times per second.
Remote renderers connect via USB and can push frames into the queue using a USB control write.
The full-speed interface should provide sufficient bandwidth to be able to keep the frame queue
filled at all times.


## Integration in SteamShovel

For further proliferation of the portable IceTop event display, the
generation of frames should was integrated into the official offline
event displayer of IceCube: Steamshovel.
This is software people have experience with, so less resistance should be
found in deploying this creation to other IceCube/IceTop groups.

