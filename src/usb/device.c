#include "usb/device.h"
#include "usb/led.h"

// USB device state machine
static enum usb_device_state_t device_state;

void set_device_state(enum usb_device_state_t state) {
  device_state = state;
  switch (state) {
    case POWERED:
    case DEFAULT:
    case ADDRESSED:
    case CONFIGURED:
      set_led_state(LED_TRIP_FAST);
      break;
    case SUSPENDED:
      set_led_state(LED_BLINK_SLOW);
      break;
    case ATTACHED:
    default:
      set_led_state(LED_OFF);
      break;
  }
}

enum usb_device_state_t get_device_state() {
  return device_state;
}
