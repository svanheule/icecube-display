#ifndef FRAME_TIMER_SOF_TRACKER_H
#define FRAME_TIMER_SOF_TRACKER_H

#include <stdint.h>

#define MS_PER_FRAME (1000/DEVICE_FPS)

uint16_t get_usb_frame_counter_value();
void new_sof_received(const uint16_t usb_frame_counter);

uint16_t get_display_frame_counter_value();
void correct_display_frame_counter(int16_t);

// abs(ms_shift) <= MS_PER_FRAME/2
void correct_display_frame_phase(const int8_t ms_shift);

void timer_rollover_callback();


#endif //FRAME_TIMER_SOF_TRACKER_H
