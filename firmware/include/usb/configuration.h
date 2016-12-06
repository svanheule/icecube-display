#ifndef USB_CONFIGURATION_H
#define USB_CONFIGURATION_H

/** \file
  * \brief USB device configuration selection.
  * \details Since currently only one device configuration is present, only two configurations
  *   are supported:
  *   - Configuration 0: USB default configuration
  *   - Configuration 1: Default device configuration with only the standard control endpoint.
  *
  * \author Sander Vanheule (Universiteit Gent)
  * \todo Add configuration 2 with an additional isochronous IN endpoint
  */

#include <stdint.h>
#include <stdbool.h>

/// Check whether the given configuration index \a index is valid.
bool valid_configuration_index(int8_t index);

/** \brief Select device configuration with index \a index.
  * \returns `true` if the new configuration was selected, `false` otherwise.
  */
bool set_configuration_index(int8_t index);

/// Get the current configuration index.
int8_t get_configuration_index();

#endif
