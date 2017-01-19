#ifndef USB_ENDPOINT_H
#define USB_ENDPOINT_H

/** \file
  * \brief Configuration of USB endpoint hardware and memory.
  * \author Sander Vanheule (Universiteit Gent)
  *
  * \defgroup usb_device USB device management
  * \brief This module groups firmware functionality related to managing the USB device state.
  * \details Any firmware implementing a new remote USB communications module will most likely
  *   require these functions.
  *   Note that currently only functionality is provided to manage different USB device
  *   configurations, but not multiple interfaces.
  *
  * \see [USB in a NutShell](http://www.beyondlogic.org/usbnutshell/usb1.shtml) for an introduction
  *   to the USB standard.
  * \see The complete [USB 2.0 specification](http://www.usb.org/developers/docs/usb20_docs/).
  */

#include <stdbool.h>
#include <stdint.h>

/** \defgroup usb_device_endpoint Endpoint management
  * \brief USB endpoint configuration and status reporting
  * \ingroup usb_device
  * @{
  * \name Endpoint configuration
  * @{
  */

/// \brief List of endpoint types.
/// \details Values of these constants corresponds to the bit values used in the USB
///   endpoint descriptors. See Table 9-13 of the USB 2.0 standard.
enum ep_type_t {
    EP_TYPE_CONTROL = 0 ///< Control endpoint
  , EP_TYPE_BULK = 2 ///< Bulk endpoint
  , EP_TYPE_INTERRUPT = 3 ///< Interrupt endoint
  , EP_TYPE_ISOCHRONOUS = 1 ///< Isochronous endpoint
};

/// Endpoint directions
enum ep_direction_t {
    EP_DIRECTION_OUT = (1 << 0) ///< OUT endpoint, for data from host to device.
  , EP_DIRECTION_IN = (1 << 1) ///< IN endpoint, for data from device to host.
  , EP_DIRECTION_BIDIR = EP_DIRECTION_IN | EP_DIRECTION_OUT ///< Bidirectional endpoint.
};

/** \brief Endpoint configuration struct.
  * \details For the default control endpoint (EP 0) the following config could be used:
  * ~~~{.c}
  * static const struct ep_hw_config_t EP_0 = {
  *     // Endpoint number 0
  *     0,
  *     // ... is a control endpoint
  *     EP_TYPE_CONTROL,
  *     // ... which is always bidirectional
  *     EP_DIRECTION_BIDIR,
  *     // ... with a 64B buffer, the maximum allowed by USB 2.0 for a full-speed device.
  *     64
  * };
  * ~~~
  */
struct ep_config_t {
  /// Endpoint number
  uint8_t num;
  /// Endpoint type
  enum ep_type_t type;
  /// Endpoint data flow direction
  enum ep_direction_t dir;
  /// Endpoint buffer size
  uint16_t size;
  /// Endpoint reset callback
  void (*init)();
};

/** \brief Initialise the USB endpoint described by \a config.
  * \details This will allocate the hardware and memory as described by the
  *   microcontroller's manual and enable the required interrupts.
  * \note Not all platforms support random endpoint allocation, so endpoints should be allocated
  *   starting from endpoint 0 up to the last endpoint.
  *   Failing to do so may result in endpoint memory corruption.
  * \returns `true` if the endpoint configuration was succesful, `false` otherwise.
  */
bool endpoint_configure(const struct ep_config_t* config);

/// Default endpoint initialisation function
void endpoint_init_default(const uint8_t ep_num);

/// Releases hardware and memory associated with the endpoint memory.
void endpoint_deconfigure(const uint8_t ep_num);

/// Return the maximum endpoint packet size.
uint16_t endpoint_get_size(const uint8_t ep_num);

/// @}


/** \name Endpoint stall
  * @{
  */

/// Stall an endpoint
bool endpoint_stall(const uint8_t ep_num);

/// Clear an endpoint stall
bool endpoint_clear_stall(const uint8_t ep_num);

/// Get endpoint stall status
bool endpoint_is_stalled(const uint8_t ep_num);

/// @}
/// \name Endpoint DATAx toggle
/// @{

/// Reset the DATAx toggle to DATA0
void endpoint_reset_data_toggle(const uint8_t ep_num);

/** \brief Get the DATAx toggle value.
  * \details For IN endpoints, this returns the next value that should be used for a data transfer.
  *     For OUT endpoints, this returns the last received value.
  *     Note that for control endpoints the returned value may be unrelated to the relevant current
  *     data toggle since these endpoints are bidirectional.
  */
uint8_t endpoint_get_data_toggle(const uint8_t ep_num);

/// @}
/// @}

#endif
