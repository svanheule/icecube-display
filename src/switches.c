#include <avr/io.h>
#include <avr/interrupt.h>
#include "switches.h"

// Use a timer to periodically check the input switches.
// Based on http://www.ganssle.com/debouncing.htm

// Currently assume all switches are on the same port
#define SWITCH_PORT_DIRECTION DDRD
#define SWITCH_PORT_OUTPUT PORTD
#define SWITCH_PORT_INPUT PIND
#define SWITCH_0_PIN DDD0
#define SWITCH_1_PIN DDD1

enum switch_state_t {
    PRESSED
  , DEPRESSED
};

static const uint8_t switch_pins[SWITCH_COUNT] = {
    SWITCH_0_PIN
  , SWITCH_1_PIN
};

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
    // Set port connected to push button to input
    SWITCH_PORT_DIRECTION &= ~(_BV(switch_pins[i]));
    // Disable internal pull-up since an external pull-up is provided
    SWITCH_PORT_OUTPUT &= ~(_BV(switch_pins[i]));
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

/// Get the (bouncy) signal from the input pins for the given switch
/// A switch depressed switch will read '1' (pull-up), a pressed switch '0'
static enum switch_state_t get_switch_signal(uint8_t switch_index) {
  uint8_t pin = switch_pins[switch_index];
  if (SWITCH_PORT_INPUT & _BV(pin)) {
    return DEPRESSED;
  }
  else {
    return PRESSED;
  }
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
        // Set pressed flag on downgoing edge
        if (get_switch_state(i) == PRESSED) {
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
