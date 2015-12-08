# (Almost) Pocket-size IceTop LED display

An LED display consisting of 81 LEDs aranged in a hexagonal shape
to represent the IceTop array, with one RGB LED per tank.


## Electronics

Using strips of 60 LEDs/m the actual spacing is rather small, resulting in
a display of about 20cm×20cm. The strips contain APA-102C LED which were
selected due to their adjustable brightness setting and clocked data input
lines. This results in less stringent timing contraints for the controlling
electronics, as the controller can decide how fast the data is clocked, and
clock jitter is less of an issue.

The display is controlled by an Arduino Uno board using, since this happened
to be laying around anyway. The firmware for the microcontrollers will be
written in C, not using the Arduino libraries, to avoid unneeded dependencies.

### Cost estimate

*Total:* €80, ordered at [lednexus](http://lednexus.de)
* 2m APA-102C 60 LEDs/m strips: €50
* 5V, 10A power supply: €30


## Microcontroller software design

The framerate for the display will need to be at least 25fps, but can in
principle be higher. The APA-102C LEDs can be clocked at up to 32MHz, so that
a new frame can typically be written out in 0.3µs. This should leave plenty of
time for the microcontroller to do other things.

The frames are stored in two buffers: one for writing out to the display, and
one for reading incoming data from the USART. Moving a byte from the USART
buffer to memory can be done in a few clock cycles. Since writing a frame can
be interrupted by pausing the data clock, the USART Rx interrupt has higher
priority than writing out a new frame and should result in little to no
noticable difference in picture quality.


## Integration in SteamShovel

For further proliferation of the portable/pocket IceTop event display, the
generation of frames should preferably be integrated into the official offline
event displayer of IceCube: SteamShovel.
This is software people have experience with, so less resistance should be
found in deploying this creation to other IceCube/IceTop groups.

