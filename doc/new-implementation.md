# Guidelines for new implementations {#new_implementations}

Using the C programming language, the icecube-display firmware project was developed with multiple
hardware implementations in mind.
A number of the core firmware modules have a generic implementation, to enable re-use on different
platforms.
For the firmware modules that are too closely tied to the hardware itself, only a header file
is provided to provide the common interface that other modules build on.

## Firmware structure
### Device initialisation
Since the firmware uses bare-metal C, microcontroller hardware initalisation has to be performed
by the user, and a memory pool for display frames should be allocated before
starting to write and generate display data.
Display firmwares should call the following initialisation routines in the listed order to prevent
firmware malfunction.

1. \ref init_display_properties() \n
  Initialises the display properties from the EEPROM data.
  Since this information is platform specific, no common implementation is provided.
2. \ref init_frame_buffers() \n
  Allocates the frame buffer memory pool. Called early to reduce chances of heap fragmentation.
  This function relies on the information initialised by init_display_properties().
3. \ref init_display_driver() \n
  Initialises the hardware required to drive the LEDs.
  Hardware dependent, so must be implemented separately for every display type.
4. \ref init_remote() \n
  Initialises the remote communications hardware and firmware.
  Strictly spoken not required if you don't want remote connectivity.
  Implementing \ref remote.h for the USB communications requires a good understanding of the
  underlying hardware. If possible, try to stick to an existing platform to avoid having to write
  the code to \ref usb_endpoint_control "drive the EP0 state machine" and properly handle the
  \ref usb/device.h "device state".
5. \ref init_frame_timer()\n
  Starts the frame timer to enable periodic clearing of the frame FIFO.
  Requires a \ref frame_timer_backend.h "frame timer backend" implementation.

### Hardware dependent features
The following files are part of the base display functionality, but are hardware specific.
An implementation should be provided by every new firmware:
* \ref display_driver.h
* \ref frame_timer_backend.h
* \ref display_properties.h
* \ref remote.h
* \ref usb/led.h
* \ref usb/address.h
* \ref usb/endpoint.h

Of course other firmware modules can be provided to extend this base functionality.
See \ref display_atmega32u4_icetop "firmware/icetop_atmega32u4" for an example with
stand-alone operation as an additional feature.

## CMake
The current firmware implementations are all designed as CMake projects.
This provides convenient out-of-source builds and provides a single interface to build and upload
firmware images:

* `make`: Compiles the firmware image.
* `make upload`: Uploads the firmware image to the device, when the device is in programming mode.
