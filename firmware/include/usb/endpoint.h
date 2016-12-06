#ifndef USB_ENDPOINT_H
#define USB_ENDPOINT_H

/** \file
  * \brief Configuration of USB endpoint hardware and memory.
  * \details Endpoint configuration is currently only focussed on control endpoints, so provisions
  *   for other endpoint types are rather minimal.
  *   For the default control endpoint (EP 0) the following config could be used:
  *   ~~~{.c}
  *   static const struct ep_hw_config_t EP_0 = {
  *       // Endpoint number 0
  *       0,
  *       // ... is a control endpoint
  *       EP_TYPE_CONTROL,
  *       // ... with a single 64B bank, the maximum allowed by USB 2.0 for a full-speed device.
  *       EP_BANK_SIZE_64 | EP_BANK_COUNT_1
  *   };
  *   ~~~
  * \author Sander Vanheule (Universiteit Gent)
  * \see [ATmega32U4 documentation §21-22](http://www.atmel.com/devices/ATMEGA32U4.aspx)
  *
  * \defgroup usb_endpoint USB endpoint operations
  * \details All communications on the USB bus happen between the host and a device endpoint.
  *   A number of functions is provided to facilitate endpoint manipulation on the microcontroller.
  *   Since the ATmega32U4 microcontroller only allows access to one endpoint at a time, an
  *   \ref usb_endpoint_stack "endpoint selection system" is provided.
  */

#include <stdbool.h>
#include <stdint.h>


/** \defgroup usb_endpoint_config Endpoint configuration
  * \ingroup usb_endpoint
  * \brief Configuration and memory allocation of USB endpoints.
  * \details To configure a USB endpoint, the USB controller needs to know what type it is and
  *   how much memory it needs.
  * @{
  */

/// List of endpoint types
/// Values of these constants corresponds to the bit values used in the USB endpoint descriptors.
/// See Table 9-13 of the USB 2.0 standard.
enum ep_type_t {
    EP_TYPE_CONTROL = 0
  , EP_TYPE_BULK = 2
  , EP_TYPE_INTERRUPT = 3
  , EP_TYPE_ISOCHRONOUS = 1
};

/// Endpoint directions
enum ep_direction_t {
    EP_DIRECTION_OUT = (1 << 0)
  , EP_DIRECTION_IN = (1 << 1)
  , EP_DIRECTION_BIDIR = EP_DIRECTION_IN | EP_DIRECTION_OUT
};

struct ep_config_t {
  /// Endpoint number
  uint8_t num;
  /// Endpoint type
  enum ep_type_t type;
  /// Endpoint data flow direction
  enum ep_direction_t dir;
  /// Endpoint buffer size
  uint16_t size;
  /// Endpoint initialisation
  void (*init)();
};

/** \brief Initialise the USB endpoint described by \a config.
  *  \par ATmega32u4 implementation
  *   This will allocate the hardware as described by the ATmega32U4 manual and enable
  *   the required interrupts. Note that endpoints should be allocated starting from endpoint 0
  *   up to the last endpoint. Failing to do so will likely result in endpoint memory corruption.
  *  \par Teensy 3.2 implementation
  *   The hardware will be configured to set up the requested endpoint, but no memory will be
  *   allocated. The buffer descriptor table (BDT) and associated memory is managed by the
  *   state machine that controlled by the endpoint interrupts.
  *
  * \returns `true` if the endpoint configuration was succesful, `false` otherwise.
  */
bool endpoint_configure(const struct ep_config_t* config);

/// Deallocates the endpoint memory.
void endpoint_deconfigure(const uint8_t ep_num);

uint16_t endpoint_get_size(const uint8_t ep_num);

/// @}


/** \defgroup usb_endpoint_status Endpoint status
  * \ingroup usb_endpoint
  * \brief Endpoint status reporting and setting.
  * \details
  * @{
  */

/// Stall an endpoint
bool endpoint_stall(const uint8_t ep_num);

/// Clear an endpoint stall
bool endpoint_clear_stall(const uint8_t ep_num);

/// Get endpoint stall status
bool endpoint_is_stalled(const uint8_t ep_num);

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

#endif