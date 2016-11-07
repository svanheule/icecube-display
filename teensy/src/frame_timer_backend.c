#include "kinetis/io.h"
#include "kinetis/pit.h"
#include "frame_timer_backend.h"

static void (*callback)();

void init_frame_timer_backend(void (*timer_callback)()) {
  // Unfreeze PIT module
  enable_pit_module();

  callback = timer_callback;

  // Disable channel and clear pending interrupts
  pit_channels[0].TCTRL = 0;
  pit_channels[0].TFLG = 1; // write 1 to clear possible pending interrupt

  // Enable NVIC IRQ
  NVIC_ENABLE_IRQ(IRQ_PIT_CH0);

  pit_channels[0].LDVAL = F_BUS/DEVICE_FPS - 1; // F_BUS = 48M if F_CPU = 48M
  pit_channels[0].TCTRL = _BV(1); // enable timer interrupts
  pit_channels[0].TCTRL = _BV(1)|_BV(0); // ... and enable timer
}

// ISR must be visible to other modules, so don't declare this static
void pit0_isr() {
  pit_channels[0].TFLG = 1;
  if (callback) {
    callback();
  }
}

timer_count_t get_counts_max() {
  return pit_channels[0].LDVAL;
}

timer_count_t get_counts_current() {
  return pit_channels[0].CVAL;
}

int8_t get_counter_direction() {
  return -1;
}

void correct_counts_max(timer_diff_t diff) {
  pit_channels[0].LDVAL += diff;
}
