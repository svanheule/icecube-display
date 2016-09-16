#include "usb/address.h"
#include <avr/io.h>

uint8_t usb_get_address() {
  return UDADDR & 0x7F;
}

void usb_set_address(uint8_t address) {
  address = address & 0x7F;
  UDADDR = address;
  UDADDR = _BV(ADDEN) | address;
}

