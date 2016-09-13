#include <kinetis.h>
#include "frame_timer.h"

// TODO Potentially merge this file with `avr/scr/frame_timer.c` 

// TODO Not hardcode the frame rate
#define FPS 25

static void (*callback)();

void init_frame_timer(void (*timer_callback)()) {
  // Unfreeze PIT module
  SIM_SCGC6 |= _BV(23); // enable PIT module clock
  PIT_MCR = _BV(1);  //disable PIT module *after* enabling PIT module clock

  PIT_TFLG0 = 1; // write 1 to clear possible pending interrupt

  PIT_LDVAL0 = F_BUS/FPS - 1; // F_BUS = 48M if F_CPU = 48M
  PIT_TCTRL0 = _BV(1); // enable timer interrupts
  PIT_TCTRL0 = _BV(1)|_BV(0); // ... and enable timer

  callback = timer_callback;

  // Enable NVIC IRQ
  NVIC_SET_PRIORITY(IRQ_PIT_CH0, 128);
  NVIC_ENABLE_IRQ(IRQ_PIT_CH0);

  PIT_MCR = 0; // enable PIT module
}

// ISR must be visible to other modules, so don't declare this static
void pit0_isr() {
  PIT_TFLG0 = 1;
  if (callback) {
    callback();
  }
}
