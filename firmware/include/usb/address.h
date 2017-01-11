#ifndef USB_ADDRESS_H
#define USB_ADDRESS_H

/** \file
  * \brief USB device address management
  * \details This interface is provided separately from device.h as its implementation is
  *   plaform dependent.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include <stdint.h>

/** \defgroup usb_device_address Address management
  * \ingroup usb_device
  * \brief USB bus address management.
  * \details Every USB device is assigned an address that is unique on the bus the device is
  *   present on.
  *   This is commonly also stored in hardware to ensure USB packets not intended for this device
  *   can be ignored.
  * @{
  */

/// Get the currently assigned USB device address
uint8_t usb_get_address();

/// Set a new USB device address
void usb_set_address(uint8_t address);

/// @}

#endif //USB_ADDRESS_H
