#include "usb/address.h"

#include "kinetis/io.h"

uint8_t usb_get_address() {
  return USB0_ADDR & 0x7F;
}

void usb_set_address(uint8_t address) {
  USB0_ADDR = address & 0x7F;
}

