#ifndef USB_CONFIGURATION_H
#define USB_CONFIGURATION_H

/** \file
  * \brief USB device configuration selection.
  * \details Since currently only one device configuration is present, only two configurations
  *   are supported:
  *   - Configuration 0: USB default configuration
  *   - Configuration 1: Default device configuration with the standard control endpoint
  *     and a bulk OUT endpoint for remote frame data transfer.
  *
  * \author Sander Vanheule (Universiteit Gent)
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
