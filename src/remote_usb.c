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
