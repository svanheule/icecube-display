#include <avr/io.h>
#include <avr/interrupt.h>
#include "frame_timer.h"

#define MODE_SELECT_A (_BV(WGM11) | _BV(WGM10))
#define MODE_SELECT_B (_BV(WGM13) | _BV(WGM12))
#define CLOCK_SELECT(n) ((n) << CS10)

enum clock_t {
    CLOCK_NONE = 0
  , CLOCK_DIV_1 = 1
  , CLOCK_DIV_8 = 2
  , CLOCK_DIV_64 = 3
  , CLOCK_DIV_256 = 4
  , CLOCK_DIV_1024 = 5
  , CLOCK_EXT_FALLING = 6
  , CLOCK_EXT_RISING = 7
};

static void (*callback)();
static timer_count_t old_max_count;

void init_frame_timer(void (*timer_callback)()) {
  /* Clock is 16MHz
   * 25 FPS: 640000 counts; prescale 64, compare (10000-1)
   * 50 FPS: 320000 counts; prescale 64, compare (5000-1)
   * Set mode to 1111 : fast PWM with OCR1A as TOP; enables double buffering for OCR1A
   */
  OCR1A = (F_CPU/64/DEVICE_FPS)-1;
  TCCR1A = MODE_SELECT_A;
  TCCR1B = MODE_SELECT_B | CLOCK_SELECT(CLOCK_DIV_64);

  old_max_count = 0;

  // Set callback and enable interrupt
  callback = timer_callback;
  TIMSK1 = (1<<OCIE1A);
}

ISR(TIMER1_COMPA_vect) {
  if (callback) {
    callback();
  }
  // If a one-time correction has been set, restore old value after roll-over.
  if (old_max_count) {
    OCR1A = old_max_count;
    old_max_count = 0;
  }
}

void restart_frame_timer() {
  TCCR1B = MODE_SELECT_B | CLOCK_SELECT(CLOCK_NONE);
  TCNT1 = 0;
  TCCR1B = MODE_SELECT_B | CLOCK_SELECT(CLOCK_DIV_64);
}

int8_t get_counter_direction() {
  return 1;
}

timer_count_t get_counts_max() {
  return OCR1A;
}

timer_count_t get_counts_current() {
  return TCNT1;
}

void correct_counts_max(timer_diff_t diff, bool is_phase_slip) {
  // Only perform correction if we are not in the middle of a phase slip
  if (!old_max_count) {
    if (is_phase_slip) {
      old_max_count = OCR1A;
    }
    else {
      old_max_count = 0;
    }

    OCR1A += diff;
  }
}
