#include "frame_timer_sof_tracker.h"
#include "frame_timer.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define MS_PER_FRAME (1000/DEVICE_FPS)

static timer_diff_t ms_steps[MS_PER_FRAME];
#define LEN_STEPS (sizeof(ms_steps)/sizeof(ms_steps[0]))
static uint8_t ms_steps_index = 0;

static timer_diff_t step_sum;
static bool step_sum_valid = false;

static void register_ms_step(timer_diff_t ms_step) {
  if (step_sum_valid) {
    step_sum -= ms_steps[ms_steps_index];
  }

  ms_steps[ms_steps_index] = ms_step;
  step_sum += ms_step;
  ms_steps_index = (ms_steps_index+1) % LEN_STEPS;

  if (ms_steps_index == 0) {
    // After wrapping around, set flag to true
    step_sum_valid = true;
  }
}

// Valid values are 0 - 0x7FF
static uint16_t current_usb_frame_counter;

uint16_t get_usb_frame_counter_value() {
  return current_usb_frame_counter;
}

void new_sof_received(const uint16_t usb_frame_counter) {
  static bool usb_frame_delta_valid = false;

  static bool ms_counts_valid = false;
  static timer_count_t previous_count;

  // Latch all counters first
  timer_count_t count = get_counts_current();

  // Calculate frame number delta
  int16_t usb_frame_delta;
  if (usb_frame_counter < current_usb_frame_counter) {
    usb_frame_delta = usb_frame_counter + (1<<11) - current_usb_frame_counter;
  }
  else {
    usb_frame_delta = usb_frame_counter - current_usb_frame_counter;
  }

  // If the counter has rolled over, the actual difference between the two counter
  // values is uncertain because this depends on the implementation of the counter
  // itself. Therefore counter difference is only used when there is no rollover.
  // This results in (MS_PER_FRAME-1) being registered per display frame. Only when
  // MS_PER_FRAME intervals have been recorded, will the frame counter be corrected.
  timer_diff_t count_diff = get_counter_direction() * (count - previous_count);
  bool counter_rolled_over = count_diff < 0;

  if (usb_frame_delta_valid && ms_counts_valid && !counter_rolled_over) {
    timer_diff_t ms_step = count_diff/usb_frame_delta;
    register_ms_step(ms_step);
  }

  if (counter_rolled_over && step_sum_valid) {
    static timer_diff_t error_accum = 0;

    // Apply correction once per rollover
    timer_diff_t error = step_sum - get_counts_max();

    // Accumulative error divider should be bigger than the mean expected error value
    // to avoid overshooting with the initial correction
    correct_counts_max( (9*error + 4*error_accum)/8 );

    error_accum += error;
  }

  current_usb_frame_counter = usb_frame_counter;
  usb_frame_delta_valid = true;

  previous_count = count;
  ms_counts_valid = true;
}
