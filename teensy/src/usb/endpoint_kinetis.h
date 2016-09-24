#ifndef USB_ENDPOINT_H
#define USB_ENDPOINT_H

#include <stdbool.h>
#include <stdint.h>

#include "kinetis/usb.h"

enum ep_type_t {
    EP_TYPE_CONTROL = 0
  , EP_TYPE_BULK = 1
  , EP_TYPE_INTERRUPT = 2
  , EP_TYPE_ISOCHRONOUS = 3
};

enum ep_direction_t {
    EP_DIRECTION_OUT = 1
  , EP_DIRECTION_IN = 2
  , EP_DIRECTION_BIDIR = 3
};

struct ep_config_t {
  uint8_t num;
  enum ep_type_t type;
  enum ep_direction_t dir;
  uint16_t size;
} __attribute__((packed));

bool endpoint_configure(const struct ep_config_t* config);

/// Deallocates the endpoint memory.
void endpoint_deconfigure(const uint8_t ep_num);

void endpoint_stall(const uint8_t ep_num);
void endpoint_clear_stall(const uint8_t ep_num);

#endif
