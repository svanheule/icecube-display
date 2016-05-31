#include "usb/endpoint.h"
#include <avr/io.h>

bool endpoint_configure(const struct ep_hw_config_t* config) {
  // Select endpoint 0
  UENUM = config->num;
  // Activate endpoint
  UECONX = _BV(EPEN);
  // Deconfigure/reset endpoint
  UECFG1X = 0;
  // Configure endpoint
  UECFG0X = config->config_type;
  UECFG1X = config->config_bank | _BV(ALLOC);
  // Enable appropriate interrupts
  switch (config->config_type >> 6) { // EP type
    case 0: // control
      UEIENX = _BV(RXSTPE);
      break;
    case 1: // isochronous
    case 2: // interrupt
    case 3: // bulk
      if (!(config->config_type & 0x1)) { // only for OUT EP
        UEIENX = _BV(RXOUTE);
      }
      break;
  }
  // Return configuration status
  return (UESTA0X & _BV(CFGOK));
}


void endpoint_deconfigure(const uint8_t ep_num) {
  const uint8_t prev_ep = UENUM;
  UENUM = ep_num;
  // Disable EP and deallocate its memory
  UECONX &= ~_BV(EPEN);
  UECFG1X &= ~_BV(ALLOC);
  UENUM = prev_ep;
}
