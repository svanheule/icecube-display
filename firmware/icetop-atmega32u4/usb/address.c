#include "usb/address.h"
#include <avr/io.h>

uint8_t usb_get_address() {
  const uint8_t tmp = UDADDR;
  return tmp & 0x80 ? tmp & 0x7F : 0;
}

void usb_set_address(uint8_t address) {
  address = address & 0x7F;
  UDADDR = address;
  UDADDR = _BV(ADDEN) | address;
}

