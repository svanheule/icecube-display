#ifndef USB_H
#define USB_H

#include "usb/std.h"

void configure_pll();
void enable_pll();
void disable_pll();


enum usb_device_state_t {
    ATTACHED
  , POWERED
  , DEFAULT
  , ADDRESSED
  , CONFIGURED
  , SUSPENDED
};

void set_device_state(enum usb_device_state_t state);
enum usb_device_state_t get_device_state();

#endif
