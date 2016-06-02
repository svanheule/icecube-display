#ifndef USB_LED_H
#define USB_LED_H

/** \file
  * \brief USB activity LED state management
  * \details The USB activity LED is controlled entirely via software, so will not turn
  *   on automatically if the power is connected, or when a firmware that is unaware of this LED
  *   is running on the device.
  *   The LED subsystem needs to be initiated by calling init_led() once, preferably as soon as
  *   possible after firmware loading. The LED can be set in three different states using
  *   set_led_state():
  *   * Off: Permanently turn off the LED
  *   * Blink (slow): Slowly blink (1Hz rate) the LED
  *   * Trip (fast): Default LED state is `on`, but a call to trip_led() can be used to shortly
  *     turn off the LED.
  *
  *   When the device is not connected via USB, the LED is set to in the _Off_ mode.
  *   If the device is connected, the state depends on whether there is an active connection or not.
  *   Slow blinking will be used to indicate that USB power is detected, but no bus activity is
  *   present (e.g. if the USB hub or PC is in stand-by mode).
  *   Trip mode is used when there is an active connection. All communication on the USB bus will
  *   be used to trip the LED, either resulting in a short blink every now an then when activity is
  *   low, or fast blinking when a lot of data is transferred.
  * \author Sander Vanheule (Universiteit Gent)
  */

/// LED state selection constants.
enum led_mode_t {
    LED_OFF //! Turn the LED off.
  , LED_BLINK_SLOW //! Slow but continous blinking mode.
  , LED_TRIP_FAST //! On by default, but can be 'tripped' (turned of shortly) with trip_led().
};

/// Initialise the USB activity LED subsystem.
void init_led();

/// Put the LED in the desired state.
void set_led_state(const enum led_mode_t mode);

/// Trip the led when the state is set to ::LED_TRIP_FAST
void trip_led();

#endif
