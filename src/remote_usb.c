#include "remote.h"
#include <stdbool.h>
#include <avr/io.h>

static void init_led();
static void set_led_state(bool on);
// TODO Timer3 ISR for blinking LED

void init_remote() {
  init_led();
}

bool is_remote_connected() {
  return false;
}


// USB_ACT LED control
static void init_led() {
  DDRB = _BV(DDB0);
  set_led_state(true);
}

static void set_led_state(bool on) {
  if (on) {
    PORTB &= ~_BV(PB0);
  }
  else {
    PORTB |= _BV(PB0);
  }
}
