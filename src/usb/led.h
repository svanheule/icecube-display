#ifndef USB_LED_H
#define USB_LED_H

#include <stdbool.h>

enum led_mode_t {
    LED_OFF
  , LED_BLINK_SLOW
  , LED_TRIP_FAST
};

void init_led();
void set_led_state(const enum led_mode_t mode);
void trip_led();

#endif
