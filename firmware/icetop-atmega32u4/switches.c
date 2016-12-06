#include <avr/io.h>
#include <avr/interrupt.h>
#include "switches.h"

// Use a timer to periodically check the input switches.
// Based on http://www.ganssle.com/debouncing.htm

#ifndef HW_REV
#error "You must define a hardware revision"
#endif

enum switch_state_t {
    PRESSED
  , DEPRESSED
};

// Hardware revision specific implementations
#define SWITCH_COUNT 2

static void init_switch_pin(uint8_t switch_index) {
  // Set port connected to push button to input
  // Disable internal pull-up since an external pull-up is provided
  switch (switch_index) {
    case 0:
      DDRD &= ~(_BV(DDD0));
      PORTD &= ~(_BV(PORTD0));
      break;
    case 1:
#if HW_REV==1
      DDRD &= ~(_BV(DDD1));
      PORTD &= ~(_BV(PORTD1));
#elif HW_REV==2
      DDRE &= ~(_BV(DDE2));
      PORTE &= ~(_BV(PORTE2));
#else
#error "Unsupported hardware revision"
#endif
      break;
    default:
      break;
  }
}

/// Get the (bouncy) signal from the input pins for the given switch
/// A switch depressed switch will read '1' (pull-up), a pressed switch '0'
static enum switch_state_t get_switch_signal(uint8_t switch_index) {
  bool depressed = true;

  switch (switch_index) {
    case 0:
      depressed = PIND & _BV(PIND0);
      break;
    case 1:
#if HW_REV==1
      depressed = PIND & _BV(PIND1);
#elif HW_REV==2
      depressed = PINE & _BV(PINE2);
#else
#error "Unsupported hardware revision"
#endif
      break;
    default:
      break;
  }

  if (depressed) {
    return DEPRESSED;
  }
  else {
    return PRESSED;
  }
}

// Launch pin read every ~5ms (200Hz)
#define SWITCH_POLLING_RATE 200

#define SWITCH_CHECK_MS (1000/SWITCH_POLLING_RATE)
#define SWITCH_TO_PRESSED_MS 10
#define SWITCH_TO_DEPRESSED_MS 100

#define SWITCH_PRESS_COUNTS (SWITCH_TO_PRESSED_MS / SWITCH_CHECK_MS)
#define SWITCH_DEPRESS_COUNTS (SWITCH_TO_DEPRESSED_MS / SWITCH_CHECK_MS)

static volatile uint8_t switch_timer[SWITCH_COUNT];
static volatile uint8_t switch_state;

static volatile uint8_t switch_edge_detected;

void init_switches() {
  for (int i = 0; i < SWITCH_COUNT; ++i) {
    // Init switch pin
    init_switch_pin(i);
    // Init countdown timers
    switch_timer[i] = SWITCH_PRESS_COUNTS;
  }

  // Reset switch state
  switch_edge_detected = 0;
  switch_state = 0;

  // Set timer mode to CTC
  TCCR0A = _BV(WGM01);
  // Use prescaler of 1024
  TCCR0B = _BV(CS02) | _BV(CS00);
  OCR0A = (uint8_t) (F_CPU/1024 / SWITCH_POLLING_RATE) - 1;
  // Enable compare match interrupt
  TIMSK0 = _BV(OCIE0A);
}

void disable_switches() {
  // Disable timer interrupt
  TIFR0 &= ~_BV(OCF0A);

  // Reset switch state
  switch_edge_detected = 0;
  switch_state = 0;
}

/// Get the debounced state for the given switch
static enum switch_state_t get_switch_state(uint8_t switch_index) {
  if (switch_state & _BV(switch_index)) {
    return PRESSED;
  }
  else {
    return DEPRESSED;
  }
}

static void toggle_switch_state(uint8_t switch_index) {
  switch_state ^= _BV(switch_index);
}

ISR(TIMER0_COMPA_vect) {
  // Debounce switches
  for (int i = 0; i < SWITCH_COUNT; ++i) {
    // If new state does not equal old state, start countdown.
    // When the new state has been stable for long enough, the counter will expire
    // and the new state can be set.
    enum switch_state_t signal = get_switch_signal(i);
    enum switch_state_t state = get_switch_state(i);
    if (state == signal) {
      if (state == DEPRESSED) { // Depressed to pressed
        switch_timer[i] = SWITCH_PRESS_COUNTS;
      }
      else { // Pressed to depressed
        switch_timer[i] = SWITCH_DEPRESS_COUNTS;
      }
    }
    else {
      // Decrement timer and check timeout
      --switch_timer[i];
      if(!switch_timer[i]) {
        toggle_switch_state(i);
        // Set pressed flag on upgoing edge
        if (get_switch_state(i) == DEPRESSED) {
          switch_edge_detected |= _BV(i);
        }
      }
    }
  }
}

bool switch_pressed(uint8_t switch_index) {
  return switch_edge_detected & _BV(switch_index);
}

void clear_switch_pressed(uint8_t switch_index) {
  switch_edge_detected &= ~_BV(switch_index);
}
