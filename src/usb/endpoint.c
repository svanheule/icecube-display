#include "usb/endpoint.h"
#include <avr/io.h>

bool endpoint_configure(const struct ep_hw_config_t* config) {
  // Select endpoint number
  endpoint_push(config->num);
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
  // Make room in stack
  endpoint_pop();
  // Return configuration status
  return (UESTA0X & _BV(CFGOK));
}


void endpoint_deconfigure(const uint8_t ep_num) {
  endpoint_push(ep_num);
  // Disable EP and deallocate its memory
  UECONX &= ~_BV(EPEN);
  UECFG1X &= ~_BV(ALLOC);
  endpoint_pop();
}

static uint8_t ep_stack[EP_STACK_DEPTH];
static uint8_t ep_stack_index = 0;

bool endpoint_push(const uint8_t ep_num) {
  if (ep_stack_index < EP_STACK_DEPTH) {
    // Put currently selected endpoint number on the stack
    ep_stack[ep_stack_index++] = UENUM;
    // Select new endpoint number
    UENUM = ep_num;
    return true;
  }
  else {
    return false;
  }
}

bool endpoint_pop() {
  if (ep_stack_index) {
    // Select previous endpoint and decrement stack count
    UENUM = ep_stack[ep_stack_index--];
    return true;
  }
  else {
    return false;
  }
}
