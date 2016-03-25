#include "usb/led.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>

// USB_ACT LED control
static enum led_mode_t led_mode;
static bool led_tripped;

static inline void set_led_on() {
  PORTB &= ~_BV(PB0);
}

static inline void set_led_off() {
  PORTB |= _BV(PB0);
}

#define INTERVALS_PER_SECOND 15

static void enable_timer(uint8_t interval_count) {
  // Two interrupts: reg A determines timer period, reg B intermediate interrupt
  OCR3A = (F_CPU/256/INTERVALS_PER_SECOND)*interval_count-1;
  OCR3B = (F_CPU/256/(2*INTERVALS_PER_SECOND))*interval_count-1;
  TCNT3 = 0;
  // Set compare mode, prescaler, and enable interrupt
  TCCR3B = _BV(WGM32) | _BV(CS32);
  TIMSK3 = _BV(OCIE3A) | _BV(OCIE3B);
}

static void disable_timer() {
  // Disable timer clock and interrupts
  TCCR3B = 0;
  TIMSK3 = 0;
}

void init_led() {
  DDRB = _BV(DDB0);
  led_tripped = false;
  set_led_state(LED_OFF);
}

void set_led_state(const enum led_mode_t mode) {
  led_mode = mode;
  switch (mode) {
    case LED_OFF:
      disable_timer();
      set_led_off();
      break;
    case LED_BLINK_SLOW:
      set_led_on();
      enable_timer(INTERVALS_PER_SECOND);
      break;
    case LED_TRIP_FAST:
      set_led_on();
      enable_timer(1);
      break;
  }
}

void trip_led() {
  led_tripped = true;
}

// LED blinking timer interrupts
ISR(TIMER3_COMPA_vect) {
  // Reset LED at end of cycle
  set_led_on();
}

ISR(TIMER3_COMPB_vect) {
  // Toggle LED if it was tripped
  if (led_mode == LED_BLINK_SLOW || led_tripped) {
    set_led_off();
    led_tripped = false;
  }
}
