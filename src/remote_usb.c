#include "remote.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#include "usb/std.h"
#include "usb/device.h"
#include "usb/configuration.h"
#include "usb/endpoint.h"
#include "usb/fifo.h"
#include "usb/descriptor.h"
#include "frame_buffer.h"
#include "usb/led.h"


// Heavily based on LUFA code, stripped down to the specifics of the ATmega32U4.

// PLL stuff
static void configure_pll() {
  // Set PLL frequency to 96MHz, output scaler to 2 to generate 48MHz USB clock
  PLLFRQ = _BV(PLLUSB) | _BV(PDIV3) | _BV(PDIV1);
  // Set input prescaler to 2 (16MHz source clock)
  PLLCSR = _BV(PINDIV);
}

static void enable_pll() {
  // Enable PLL
  PLLCSR |= _BV(PLLE);
  // Wait for PLL lock
  while (!(PLLCSR & _BV(PLOCK))) {}
}

static void disable_pll() {
  PLLCSR &= ~_BV(PLLE);
}


void init_remote() {
  /* Reset all configurations to their intended state while the device is detatched.
   * Then, when everything is set up, enable the end-of-reset interrupt en attach the device.
   * If the device was already connected to the host, this will immediatly result in an interrupt,
   * thus a full configuration of the device. Otherwise, the device will be fully configured once
   * the host is connected. This ensures that the end-of-reset ISR may always assume the same
   * initial state.
   */
  USBCON = _BV(FRZCLK);
  UHWCON = 0;
  UDIEN = 0;
  // Clear all interrupts
  USBINT = 0;
  UDINT = 0;
  // -- End of state reset --

  // Enable regulator
  UHWCON = _BV(UVREGE);

  // Configure PLL
  configure_pll();
  disable_pll();
  // Enable USB interface
  USBCON = _BV(FRZCLK) | _BV(OTGPADE) | _BV(USBE) | _BV(VBUSTE);
  // TODO Decide which interrupts to enable
//  UDIEN = _BV(EORSTE) | _BV(WAKEUPE);

  // Initialise USB activity LED
  init_led();
  // Prepare to enter ATTACHED state

  // Attach pull-up
  UDCON &= ~_BV(DETACH);
  set_device_state(ATTACHED);
}

bool is_remote_connected() {
  // Communication is only possible in a configured state
  return get_device_state() == CONFIGURED;
}

#define FLAG_IS_SET(reg, flag) (reg & (1<<flag))
#define CLEAR_USBINT(flag) USBINT &= ~(1<<flag)
#define CLEAR_UDINT(flag) UDINT &= ~(1<<flag)
#define CLEAR_FLAG(reg, flag) reg &= ~(1<<flag)
#define SET_FLAG(reg, flag) reg |= (1<<flag)

ISR(USB_GEN_vect) {
  // VBUS transitions
  if (FLAG_IS_SET(USBINT, VBUSTI) && FLAG_IS_SET(USBCON, VBUSTE)) {
    CLEAR_USBINT(VBUSTI);
    if (FLAG_IS_SET(USBSTA, VBUS)) {
      // TODO reset all endpoints
      enable_pll();
      CLEAR_FLAG(USBCON, FRZCLK);
      SET_FLAG(UDIEN, EORSTE);
      SET_FLAG(UDIEN, SUSPI);
      set_device_state(POWERED);
    }
    else {
      CLEAR_FLAG(UDIEN, SUSPI);
      CLEAR_FLAG(UDIEN, WAKEUPE);
      CLEAR_FLAG(UDIEN, EORSTE);
      SET_FLAG(USBCON, FRZCLK);
      disable_pll();
      set_device_state(ATTACHED);
    }
  }

  // End-of-reset
  if (FLAG_IS_SET(UDINT, EORSTI) && FLAG_IS_SET(UDIEN, EORSTE)) {
    CLEAR_UDINT(EORSTI);
    // After reset, wait for activity
    CLEAR_UDINT(WAKEUPI);
    CLEAR_FLAG(UDIEN, WAKEUPE);
    SET_FLAG(UDIEN, SUSPE);

    // Load default configuration
    if (set_configuration_index(0)) {
      set_device_state(DEFAULT);
    }
  }

  // Wake-up, suspend
  if (FLAG_IS_SET(UDINT, SUSPI) && FLAG_IS_SET(UDIEN, SUSPE)) {
    CLEAR_FLAG(UDIEN, SUSPE);
    CLEAR_UDINT(WAKEUPI);
    SET_FLAG(UDIEN, WAKEUPE);

    SET_FLAG(USBCON, FRZCLK);
    disable_pll();
    set_device_state(SUSPENDED);
  }

  if (FLAG_IS_SET(UDINT, WAKEUPI) && FLAG_IS_SET(UDIEN, WAKEUPE)) {
    // Wake up device
    enable_pll();
    CLEAR_FLAG(USBCON, FRZCLK);

    // Clear WAKEUP, enable SUSPEND
    CLEAR_UDINT(SUSPI);
    CLEAR_UDINT(WAKEUPI);
    CLEAR_FLAG(UDIEN, WAKEUPE);
    SET_FLAG(UDIEN, SUSPE);

    // Track device state
    if (get_configuration_index() > 0) {
      set_device_state(CONFIGURED);
    }
    else if (FLAG_IS_SET(UDADDR, ADDEN)) {
      set_device_state(ADDRESSED);
    }
    else if (get_configuration_index() == 0) {
      set_device_state(DEFAULT);
    }
    else {
      set_device_state(POWERED);
    }
  }
}
