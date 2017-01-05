#include "usb/endpoint.h"
#include "usb/endpoint_stack.h"
#include "usb/endpoint_fifo.h"
#include <avr/io.h>

#define EP_TYPE_MASK (3<<6)
#define EP_DIR_MASK 1

#define AVR_EP_TYPE_CONTROL 0
#define AVR_EP_TYPE_ISOCHRONOUS (1<<EPTYPE0)
#define AVR_EP_TYPE_BULK (2<<EPTYPE0)
#define AVR_EP_TYPE_INTERRUPT (3<<EPTYPE0)

#define AVR_EP_DIR_IN 1
#define AVR_EP_DIR_OUT 0

#define MAX_EP_NUM 6

/* An endpoint can use either one or two buffers. Using only one bank saves memory,
 * but also requires the buffer to be emptied before the endpoint can resume operation.
 * With two banks, the buffers are used in a ping-pong fashion, allowing for simultaneous use
 * of the buffers by the endpoint hardware and the firmware. While one buffer is used by the
 * hardware, the firmware can read/write to the other. This may allow for higher throughputs as
 * the endpoint doesn't have to wait for the firmware to finish to transmit or receive more data.
 */

bool endpoint_configure(const struct ep_config_t* config) {
  // EP numbers larger than 6 are not supported
  if (config->num > MAX_EP_NUM) {
    return false;
  }

  // Select endpoint number
  endpoint_push(config->num);
  uint8_t log2_bank_size = 0;
  uint16_t bank_size = (config->size-1) / 8;
  while (bank_size) {
    ++log2_bank_size;
    bank_size >>= 1;
  }

  bool config_ok = true;
  uint8_t cfg0 = 0;
  uint8_t cfg1 = ((log2_bank_size & 0x7) << EPSIZE0);

  switch (config->type) {
    case EP_TYPE_CONTROL:
      cfg0 = AVR_EP_TYPE_CONTROL;
      break;
    case EP_TYPE_BULK:
      cfg0 = AVR_EP_TYPE_BULK;
      break;
    case EP_TYPE_INTERRUPT:
      cfg0 = AVR_EP_TYPE_INTERRUPT;
      break;
    case EP_TYPE_ISOCHRONOUS:
      cfg0 = AVR_EP_TYPE_ISOCHRONOUS;
      break;
  }

  if (config->type == EP_TYPE_CONTROL) {
    if (config->dir != EP_DIRECTION_BIDIR) {
      config_ok = false;
    }
  }
  else {
    // Use double banks
    cfg1 |= _BV(EPBK0);
    if (config->dir == EP_DIRECTION_IN) {
      cfg0 |= AVR_EP_DIR_IN;
    }
    else if (config->dir == EP_DIRECTION_BIDIR) {
      config_ok = false;
    }
  }

  if (config_ok) {
    // Deconfigure/reset endpoint
    UECFG1X = 0;
    UERST |= _BV(config->num);
    UERST &= ~_BV(config->num);
    // Activate endpoint and reset data toggle
    UECONX = _BV(EPEN) | _BV(RSTDT);
    UECFG0X = cfg0;
    UECFG1X = cfg1 | _BV(ALLOC);

    // Enable appropriate interrupts
    if (config->type == EP_TYPE_CONTROL) {
      UEIENX = _BV(RXSTPE);
    }
    else if (config->dir == EP_DIRECTION_OUT) {
      UEIENX = _BV(RXOUTE);
    }

    config_ok = UESTA0X & _BV(CFGOK);
  }
  // Make room in stack
  endpoint_pop();
  // Return configuration status
  return config_ok;
}


void endpoint_deconfigure(const uint8_t ep_num) {
  endpoint_push(ep_num);
  // Disable EP and deallocate its memory
  UECONX &= ~_BV(EPEN);
  UECFG1X &= ~_BV(ALLOC);
  endpoint_pop();
}

uint16_t endpoint_get_size(const uint8_t ep_num) {
  uint16_t size = 0;
  if (endpoint_push(ep_num)) {
    size = fifo_size();
    endpoint_pop();
  }
  return size;
}

bool endpoint_stall(const uint8_t ep_num) {
  if (endpoint_push(ep_num)) {
    bool can_stall = UECONX & _BV(EPEN);
    if (can_stall) {
      UECONX |= _BV(STALLRQ);
    }
    endpoint_pop();
    return can_stall;
  }
  return false;
}

bool endpoint_clear_stall(const uint8_t ep_num) {
  if (endpoint_push(ep_num)) {
    bool can_stall = UECONX & _BV(EPEN);
    if (can_stall) {
      UECONX |= _BV(STALLRQC);
      UERST |= _BV(ep_num);
      UERST &= ~_BV(ep_num);
    }
    endpoint_pop();
    return can_stall;
  }
  return false;
}

bool endpoint_is_stalled(const uint8_t ep_num) {
  bool stalled = false;
  if (endpoint_push(ep_num)) {
    stalled = (UECONX & _BV(EPEN)) && (UECONX & _BV(STALLRQ));
    endpoint_pop();
  }
  return stalled;
}

// Data toggle functions
void endpoint_reset_data_toggle(const uint8_t ep_num) {
  if (endpoint_push(ep_num)) {
    if (UECONX & _BV(EPEN)) {
      UECONX |= _BV(RSTDT);
    }
    endpoint_pop();
  }
}

uint8_t endpoint_get_data_toggle(const uint8_t ep_num) {
  uint8_t data_toggle = 0;
  if (endpoint_push(ep_num)) {
    data_toggle = (UESTA0X & _BV(DTSEQ0)) >> DTSEQ0;
    endpoint_pop();
  }
  return data_toggle;
}
