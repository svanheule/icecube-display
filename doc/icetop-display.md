# IceTop LED event display # {#icetop-display}

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

The picture below shows the geometry of the detector, as well as the 3 in-fill stations in black.
The LED display mimics this geometry, by putting each station on a regular hexagonal grid, instead
of the slightly irregular grid of the actual detector.
The in-fill stations have not been included in the small display due to a lack of space for the LEDs.

\image html geometry/icetop-array.png "IceTop detector array layout"
\image latex geometry/icetop-array.pdf "IceTop detector array layout" width=0.7\textwidth

## Display usage ##

The display has a number of input buttons and LEDs to interact with and relay status information to
the user.
The green *Power* LED indicates whether the display is receiving 5V power input.
The orange *USB activity* LED will light up when there is a USB connection, and blink when there
is bus activity, e.g. when display data is transmitted.
Aside from Steamshovel connectivity, this display also supports stand-alone operation.

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

