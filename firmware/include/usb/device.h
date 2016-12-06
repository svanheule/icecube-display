#ifndef USB_H
#define USB_H

/** \file
  * \brief USB device state management
  * \author Sander Vanheule (Universiteit Gent)
  */

/// USB device states as defined by §9.1 of the
/// [USB 2.0 specification](http://www.usb.org/developers/docs/usb20_docs/).
enum usb_device_state_t {
  /// USB hardware is initialised
  ATTACHED,
  /// USB VBUS is connected, i.e. th 5V USB power supply is present.
  POWERED,
  /// The USB device is configured with only the default endpoint zero, and address zero.
  DEFAULT,
  /// The USB device is configured with only the default endpoint zero, and a unique address.
  ADDRESSED,
  /// The USB device is in a device defined configuration.
  CONFIGURED,
  /// The USB device is suspended, i.e. no bus communication has been seen for at least 3ms.
  SUSPENDED
};

/** \brief Set the USB device state.
  * \note Since the USB activity LED behaviour (see usb/led.h) is linked to the device state,
  *   a call to this function also sets the appropriate state for the LED:
  *   - ::ATTACHED: LED off
  *   - ::SUSPENDED: blink
  *   - other states: LED on (tripped)
  *
  * \param state The new state.
  */
void set_device_state(enum usb_device_state_t state);

/// Get the current USB device state.
enum usb_device_state_t get_device_state();

#endif