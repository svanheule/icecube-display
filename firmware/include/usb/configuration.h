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

/** \defgroup usb_device_configuration Configuration management
  * \ingroup usb_device
  * \brief Manage USB device endpoint configurations
  * \details
  * @{
  */

/// Check whether the given configuration index \a index is valid.
/// Index 0 will always return `true` as this is required by all USB devices.
bool valid_configuration_index(int8_t index);

/** Select device configuration with index \a index.
  * \returns `true` if the new configuration was selected, `false` otherwise.
  */
bool set_configuration_index(int8_t index);

/// Get the current configuration index.
/// \returns -1 if no configuration was previously selected, 0 or a positive value otherwise.
int8_t get_configuration_index();


struct configuration_t {
  uint8_t endpoint_count;
  const struct ep_config_t* ep_config_list;
};

const struct configuration_t* get_configuration_P(int8_t index);

/// @}

#endif
