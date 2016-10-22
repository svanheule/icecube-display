#include "frame_timer_sof_tracker.h"
#include "frame_timer.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define MS_PER_FRAME (1000/DEVICE_FPS)

static int32_t ms_steps[MS_PER_FRAME];
#define LEN_STEPS (sizeof(ms_steps)/sizeof(ms_steps[0]))
static uint8_t ms_steps_index = 0;

static int32_t step_sum;
static bool step_sum_valid = false;

static void register_ms_step(int32_t ms_step) {
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

#include <stdalign.h>

static void histogram_add(struct histogram_t* hist, ptrdiff_t bin) {
  // Clamp bin index
  if (bin < 0) {
    bin = 0;
  }
  else if (bin > hist->bin_count-1) {
    bin = hist->bin_count-1;
  }

  // Automatic histogram reset if full
  if (hist->bins[bin] == UINT16_MAX) {
    memset(hist->bins, 0, sizeof(uint16_t)*hist->bin_count);
  }

  hist->bins[bin]++;
}

#define NOMINAL_MS_COUNTS (F_CPU/1000)
#define MS_COUNTS_BIN_WIDTH 1

#define BINS_HIST_ERROR 64
static uint16_t alignas(4) error_bins[BINS_HIST_ERROR];
struct histogram_t histogram_error = {BINS_HIST_ERROR, &error_bins[0]};

#define BINS_HIST_MS_COUNTS 64
static uint16_t alignas(4) ms_counts_bins[BINS_HIST_MS_COUNTS];
struct histogram_t histogram_ms_counts = {BINS_HIST_MS_COUNTS, &ms_counts_bins[0]};

static bool usb_frame_delta_valid = false;
static bool ms_counts_valid = false;

static int32_t error_accum = 0;

void new_sof_received(const uint16_t usb_frame_counter) {
  static uint16_t previous_usb_frame_counter;
  static uint32_t previous_count;

  // Latch all counters first
  uint32_t count = get_counts_current();

  // Calculate frame number delta and store in histogram if there is space left
  int16_t usb_frame_delta;
  if (usb_frame_counter < previous_usb_frame_counter) {
    usb_frame_delta = usb_frame_counter + (1<<11) - previous_usb_frame_counter;
  }
  else {
    usb_frame_delta = usb_frame_counter - previous_usb_frame_counter;
  }

  previous_usb_frame_counter = usb_frame_counter;

  // If the counter has rolled over, the actual difference between the two counter
  // values is uncertain because this depends on the implementation of the counter
  // itself. Therefore counter difference is only used when there is no rollover.
  int32_t count_diff = get_counter_direction() * (count - previous_count);
  bool counter_rolled_over = count_diff < 0;

  if (usb_frame_delta_valid && ms_counts_valid && !counter_rolled_over) {
    int32_t ms_step = count_diff/usb_frame_delta;
    int32_t bin_diff = (ms_step - NOMINAL_MS_COUNTS + MS_COUNTS_BIN_WIDTH/2) / MS_COUNTS_BIN_WIDTH;
    histogram_add(&histogram_ms_counts, BINS_HIST_MS_COUNTS/2 + bin_diff);

    register_ms_step(ms_step);
  }

  if (counter_rolled_over) {
    // Apply correction once per rollover
      int32_t error = step_sum - get_counts_max();
      histogram_add(&histogram_error, BINS_HIST_ERROR/2 + error/8);

      error_accum += error;
      // Accumulative error divider should be bigger than the mean expected error value
      // to avoid overshooting with the initial correction
      correct_counts_max((error + error_accum/16) / 4);
  }

  previous_count = count;
  usb_frame_delta_valid = true;
  ms_counts_valid = true;
}

