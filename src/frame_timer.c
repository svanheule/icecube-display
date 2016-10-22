#include <avr/io.h>
#include <avr/interrupt.h>

#define MODE_SELECT_A (_BV(WGM11) | _BV(WGM10))
#define MODE_SELECT_B (_BV(WGM13) | _BV(WGM12))
#define CLOCK_SELECT(n) ((n) << CS10)

static void (*callback)();

void init_frame_timer(void (*timer_callback)()) {
  /* Clock is 16MHz
   * 25 FPS: 640000 counts; prescale 64, compare (10000-1)
   * 50 FPS: 320000 counts; prescale 64, compare (5000-1)
   * Set mode to 1111 : fast PWM with OCR1A as TOP; enables double buffering for OCR1A
   */
  OCR1A = (F_CPU/64/DEVICE_FPS)-1;
  TCCR1A = MODE_SELECT_A;
  TCCR1B = MODE_SELECT_B | CLOCK_SELECT(3);

  // Set callback and enable interrupt
  callback = timer_callback;
  TIMSK1 = (1<<OCIE1A);
}

ISR(TIMER1_COMPA_vect) {
  if (callback) {
    callback();
  }
}

void restart_frame_timer() {
  TCCR1B = MODE_SELECT_B;
  TCNT1 = 0;
  TCCR1B = MODE_SELECT_B | CLOCK_SELECT(3);
}

int8_t get_counter_direction() {
  return 1;
}

uint32_t get_counts_max() {
  return OCR1A;
}

uint32_t get_counts_current() {
  return TCNT1;
}

void correct_counts_max(int32_t diff) {
  OCR1A += diff;
}
