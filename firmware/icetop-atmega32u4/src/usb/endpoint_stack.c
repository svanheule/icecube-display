#include "usb/endpoint_stack.h"
#include <avr/io.h>

static uint8_t ep_stack[EP_STACK_DEPTH];
static uint8_t ep_stack_index = 0;

#define MAX_EP_NUM 6

bool endpoint_push(const uint8_t ep_num) {
  if (ep_num <= MAX_EP_NUM && ep_stack_index < EP_STACK_DEPTH) {
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
