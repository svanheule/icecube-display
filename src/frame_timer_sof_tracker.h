#ifndef FRAME_TIMER_SOF_TRACKER_H
#define FRAME_TIMER_SOF_TRACKER_H

#include <stdint.h>

void new_sof_received(uint16_t sof_count);

struct histogram_t {
  uint16_t bin_count;
  uint16_t* bins;
};

#endif //FRAME_TIMER_SOF_TRACKER_H
