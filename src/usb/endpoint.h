#ifndef USB_ENDPOINT_H
#define USB_ENDPOINT_H

#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>

struct ep_hw_config_t {
  uint8_t num;
  uint8_t cfg0;
  uint8_t cfg1;
};

bool endpoint_configure(const struct ep_hw_config_t* config);

#endif
