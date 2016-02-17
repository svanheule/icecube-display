#include "remote.h"
#include <stdbool.h>
#include <avr/io.h>

static void init_led();
static void set_led_state(bool on);
// TODO Timer3 ISR for blinking LED

static void init_usb();
// TODO USB ISRs

void init_remote() {
  init_led();
}

bool is_remote_connected() {
  return false;
}


// USB_ACT LED control
static void init_led() {
  DDRB = _BV(DDB0);
  set_led_state(true);
}

static void set_led_state(bool on) {
  if (on) {
    PORTB &= ~_BV(PB0);
  }
  else {
    PORTB |= _BV(PB0);
  }
}

// USB subsystem definitions
#include "usb.h"
#include <avr/pgmspace.h>

#define EP_SIZE_0 64
#define EP_SIZE_1 128
static const uint8_t EP_SIZE[6] PROGMEM = {EP_SIZE_0, 0, 0, 0, 0, 0};

#define BCD_DEVICE_VERSION 0x0010

static const UsbDeviceDescriptor DEVICE_DESCRIPTOR PROGMEM = {
    {sizeof(UsbConfigurationDescriptor), DEVICE}
  , BCD_USB_VERSION
  , 0
  , 0
  , 0
  , EP_SIZE_0
  , ID_VENDOR
  , ID_PRODUCT
  , BCD_DEVICE_VERSION
  , 0
  , 0
  , 0
  , 0
};

static const UsbConfigurationDescriptor CONFIGURATION_DESCRIPTOR PROGMEM = {
    {sizeof(UsbConfigurationDescriptor), CONFIGURATION}
  , sizeof(UsbConfigurationDescriptor) + sizeof(UsbInterfaceDescriptor)
  , 1
  , 1
  , 0
  , _BV(7) | _BV(6)
  , 49
};

static const UsbInterfaceDescriptor INTERFACE_DESCRIPTOR PROGMEM = {
    {sizeof(UsbInterfaceDescriptor), INTERFACE}
  , 0
  , 0
  , 0
  , 0xFF
  , 0
  , 0
  , 0
};
