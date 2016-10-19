#include <avr/io.h>
#include <avr/interrupt.h>

static void (*callback)();

void init_frame_timer(void (*timer_callback)()) {
  /* Clock is 16MHz
   * 25 FPS: 640000 counts; prescale 256, compare (2500-1)
   * 50 FPS: 320000 counts; prescale 256, compare (1250-1)
   * Prescale factor is 2^(n+2) with n = CS12:CS11:CS10
   * Set mode to 0100 : CTC with compare to OCR1A
   */
  OCR1A = (F_CPU/256/25)-1;
  // Set compare mode, prescaler, and enable interrupt
  TCCR1B = (1<<WGM12) | (1<<CS12);

  callback = timer_callback;
  TIMSK1 = (1<<OCIE1A);
}

ISR(TIMER1_COMPA_vect) {
  if (callback) {
    callback();
  }
}
