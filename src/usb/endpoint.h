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

enum ep_type_t {
    EP_CONTROL
  , EP_ISOCHRONOUS_IN
  , EP_ISOCHRONOUS_OUT
  , EP_INTERRUPT_IN
  , EP_INTERRUPT_OUT
  , EP_BULK_IN
  , EP_BULK_OUT
};

struct ep_config_t {
  uint8_t num;
  uint16_t size;
  enum ep_type_t type;
  // TODO isochronous flags
  // TODO interval
};

bool endpoint_configure(const struct ep_hw_config_t* config);

#endif
