#include "usb/device.h"
#include <avr/io.h>

// PLL stuff
void configure_pll() {
  // Set PLL frequency to 96MHz, output scaler to 2 to generate 48MHz USB clock
  PLLFRQ = _BV(PLLUSB) | _BV(PDIV3) | _BV(PDIV1);
  // Set input prescaler to 2 (16MHz source clock)
  PLLCSR = _BV(PINDIV);
}

void enable_pll() {
  // Enable PLL
  PLLCSR |= _BV(PLLE);
  // Wait for PLL lock
  while (!(PLLCSR & _BV(PLOCK))) {}
}

void disable_pll() {
  PLLCSR &= ~_BV(PLLE);
}

// USB device state machine
static enum usb_device_state_t device_state;

void set_device_state(enum usb_device_state_t state) {
  device_state = state;
}

enum usb_device_state_t get_device_state() {
  return device_state;
}
