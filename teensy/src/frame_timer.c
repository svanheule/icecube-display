#include "frame_timer_backend.h"
#include "frame_timer.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdatomic.h>

/* FRAME COUNTER */
static atomic_flag frame_counter_phase_lock;
static struct display_frame_usb_phase_t frame_counter_phase;

bool get_display_frame_usb_phase(struct display_frame_usb_phase_t* counter_phase) {
  if (!atomic_flag_test_and_set(&frame_counter_phase_lock)) {
    *counter_phase = frame_counter_phase;
    atomic_flag_clear(&frame_counter_phase_lock);
    return true;
  }
  else {
    return false;
  }
}

bool correct_display_frame_counter(const int16_t frame_diff) {
  if (!atomic_flag_test_and_set(&frame_counter_phase_lock)) {
    frame_counter_phase.display_frame_counter += frame_diff;
    atomic_flag_clear(&frame_counter_phase_lock);
    return true;
  }
  else {
    return false;
  }
}


/* MAIN LOOP FRAME DISPLAY CONTROL */
static void timer_rollover_callback();
static volatile atomic_bool draw_frame;

void init_frame_timer() {
  atomic_init(&draw_frame, false);

  frame_counter_phase.display_frame_counter = 0;
  frame_counter_phase.usb_frame_counter = 0;
  atomic_flag_clear(&frame_counter_phase_lock);

  init_frame_timer_backend(timer_rollover_callback);
}

bool should_draw_frame() {
  return draw_frame;
}

void clear_draw_frame() {
  draw_frame = false;
}


/* CLOCK TICKS PER MS RUNNING AVERAGE
 * By keeping a running sum of the number of device clock tick in MS_PER_FRAME milliseconds,
 * a fairly accurate estimate of the frame timer maximum value can be obtained that is updated
 * every time a USB SOF token is received.
 */

static timer_diff_t ms_steps[MS_PER_FRAME];
#define LEN_STEPS (sizeof(ms_steps)/sizeof(ms_steps[0]))
static uint8_t ms_steps_index = 0;

static timer_count_t step_sum;
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

// Calculate the number of clock ticks in the provided number of ms.
static timer_count_t get_ms_tick_count(uint8_t ms) {
  timer_count_t total_count = 0;

  while (ms--) {
    uint8_t index = (ms_steps_index - ms + LEN_STEPS) % LEN_STEPS;
    total_count += ms_steps[index];
  }

  return total_count;
}


/* FRAME DISPLAY PHASE */
static timer_diff_t phase_slip_correction = 0;

void correct_display_frame_phase(const int8_t ms_shift) {
  if (ms_shift < 0) {
    phase_slip_correction = -get_ms_tick_count(-ms_shift);
  }
  else if (ms_shift > 0){
    phase_slip_correction = get_ms_tick_count(ms_shift);
  }
}


/* USB SOF TRACKING */
#include <util/atomic.h>

// Valid values are 0 - 0x7FF
static uint16_t current_usb_frame_counter;

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

  // Since using _Atomic uint16_t gives linking errors on AVR, work around it by using an
  // atomic block.
  // This completely disables interrupts while writing this variable, which is overkill for
  // ARM, but at least ensures reads of this variable will always return a correct value.
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    current_usb_frame_counter = usb_frame_counter;
  }
  usb_frame_delta_valid = true;

  previous_count = count;
  ms_counts_valid = true;
}


/* FRAME TIMER FSM */
enum correction_state_t {
    TRACK
  , PHASE_SLIP
  , PHASE_SLIP_RESTORE
};

static void timer_rollover_callback() {
  // The swap should *never* return true, but check nevertheless.
  if(!atomic_flag_test_and_set(&frame_counter_phase_lock)) {
    // Increment display frame counter on each rollover and latch usb frame counter value
    frame_counter_phase.display_frame_counter++;
    frame_counter_phase.usb_frame_counter = current_usb_frame_counter;
    atomic_flag_clear(&frame_counter_phase_lock);
  }
  draw_frame = true;

  // Use a small state machine to implement SOF frequency tracking and phase shifting
  static enum correction_state_t correction_state = TRACK;

  // If a phase slip was set, go through a cycle of slip and restore before returning
  // to SOF tracking
  if (correction_state == TRACK && phase_slip_correction != 0) {
    correction_state = PHASE_SLIP;
  }

  switch (correction_state) {
    case TRACK:
      if (step_sum_valid) {
        static timer_diff_t error_accum = 0;
        // Apply correction once per rollover
        timer_diff_t error = step_sum - get_counts_max();
        // Accumulative error divider should be bigger than the mean expected error value
        // to avoid overshooting with the initial correction
        correct_counts_max((9*error + 4*error_accum)/8);
        error_accum += error;
      }
      break;
    case PHASE_SLIP:
      correct_counts_max(phase_slip_correction);
      correction_state = PHASE_SLIP_RESTORE;
      break;
    case PHASE_SLIP_RESTORE:
      correct_counts_max(-phase_slip_correction);
      phase_slip_correction = 0;
      correction_state = TRACK;
      break;
  }
}
