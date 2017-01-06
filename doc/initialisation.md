# Display initialisation {#display_initialisation}

In order to properly initialise the display, the microcontroller hardware has to
be properly configured and a memory pool for display frames should be allocated before
starting to write and generate display data.

Display firmwares should call the following initialisation routines in the listed order to prevent
firmware malfunction.

1. \ref init_display_properties():
   Initialises the display properties from the EEPROM data.
2. \ref init_frame_buffers():
   Allocates the frame buffer memory pool. Called early to reduce chances of heap fragmentation.
3. \ref init_display_driver():
   Initialises the hardware required to drive the LEDs.
4. \ref init_remote():
   Initialises the remote communications hardware and firmware.
5. \ref init_frame_timer():
   Starts the frame timer.
