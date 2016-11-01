#ifndef FRAME_TIMER_SOF_TRACKER_H
#define FRAME_TIMER_SOF_TRACKER_H

#include <stdint.h>

void new_sof_received(const uint16_t usb_frame_counter);

struct histogram_t {
  uint16_t bin_count;
  uint16_t* bins;
};

uint16_t get_usb_frame_counter_value();

#endif //FRAME_TIMER_SOF_TRACKER_H
