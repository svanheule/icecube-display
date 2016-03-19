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
  UECFG0X = config->cfg0;
  UECFG1X = config->cfg1 | _BV(ALLOC);
  // Enable appropriate interrupts
  switch (config->cfg0 >> 6) { // EP type
    case 0: // control
      UEIENX = _BV(RXSTPE);
      break;
    case 1: // isochronous
    case 2: // interrupt
    case 3: // bulk
      if (!(config->cfg0 & 0x1)) { // only for OUT EP
        UEIENX = _BV(RXOUTE);
      }
      break;
  }
  // Return configuration status
  return (UESTA0X & _BV(CFGOK));
}
