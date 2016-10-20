#include <avr/io.h>
#include <avr/interrupt.h>

static void (*callback)();

void init_frame_timer(void (*timer_callback)()) {
  /* Clock is 16MHz
   * 25 FPS: 640000 counts; prescale 64, compare (10000-1)
   * 50 FPS: 320000 counts; prescale 64, compare (5000-1)
   * Set mode to 0100 : CTC with compare to OCR1A
   */
  OCR1A = (F_CPU/64/25)-1;
  // Set compare mode, prescaler, and enable interrupt
  TCCR1B = (1<<WGM12) | (3 << CS10);

  callback = timer_callback;
  TIMSK1 = (1<<OCIE1A);
}

ISR(TIMER1_COMPA_vect) {
  if (callback) {
    callback();
  }
}
