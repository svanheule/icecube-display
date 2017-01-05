#ifndef USB_ADDRESS_H
#define USB_ADDRESS_H

/** \file
  * \brief USB device address management
  * \details This interface is provided separately from device.h as its implementation is
  *   plaform dependent.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include <stdint.h>

/// Get the currently assigned USB device address
uint8_t usb_get_address();

/// Set a new USB device address
void usb_set_address(uint8_t address);

#endif //USB_ADDRESS_H
