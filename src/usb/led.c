#include "led.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>

// USB_ACT LED control
static bool led_tripped;
static void set_led_state(bool on);

static void init_timer() {
  // Two interrupts: reg A determines timer period, reg B intermediate interrupt
  OCR3A = F_CPU/256/20-1;
  OCR3B = F_CPU/256/40-1;
  // Set compare mode, prescaler, and enable interrupt
  TCCR3B = _BV(WGM32) | _BV(CS32) | _BV(CS30);
  TIMSK3 = _BV(OCIE3A) | _BV(OCIE3B);
}

void init_led() {
  DDRB = _BV(DDB0);
  led_tripped = false;
  set_led_state(false);
  init_timer();
}

void set_led_state(bool on) {
  if (on) {
    PORTB &= ~_BV(PB0);
  }
  else {
    PORTB |= _BV(PB0);
  }
}

void trip_led() {
  led_tripped = true;
}

// LED blinking timer interrupts
ISR(TIMER3_COMPA_vect) {
  // Reset LED at end of cycle
  set_led_state(true);
}

ISR(TIMER3_COMPB_vect) {
  // Toggle LED if it was tripped
  if (led_tripped) {
    set_led_state(false);
    led_tripped = false;
  }
}

