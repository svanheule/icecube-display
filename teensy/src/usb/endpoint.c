#include "usb/endpoint_kinetis.h"
#include "kinetis/io.h"

#define ENDPOINT_REGISTER_ADDRESS(i) ((volatile uint8_t*)(&(USB0_ENDPT0) + 4*i))

bool endpoint_configure(const struct ep_config_t* config) {
  uint8_t reg = 0;

  if (config->type == EP_TYPE_CONTROL && config->dir == EP_DIRECTION_BIDIR) {
    reg = USB_ENDPT_EPTXEN | USB_ENDPT_EPRXEN;
  }
  else {
    // No handshaking for isochronous endpoints
    if (config->type != EP_TYPE_ISOCHRONOUS) {
      reg |= USB_ENDPT_EPHSHK;
    }
    // Disable setup tokens for bidirectional non-control endpoints
    switch (config->dir) {
      case EP_DIRECTION_IN:
        reg |= USB_ENDPT_EPTXEN;
        break;
      case EP_DIRECTION_OUT:
        reg |= USB_ENDPT_EPRXEN;
        break;
      case EP_DIRECTION_BIDIR:
        reg |= USB_ENDPT_EPCTLDIS | USB_ENDPT_EPRXEN | USB_ENDPT_EPTXEN;
        break;
    }
  }

  // Memory is currently allocated in remote_dummy.c

  *ENDPOINT_REGISTER_ADDRESS(config->num) = reg;
  return reg != 0;
}

void endpoint_deconfigure(const uint8_t ep_num) {
  *ENDPOINT_REGISTER_ADDRESS(ep_num) = 0;
}

void endpoint_stall(const uint8_t ep_num) {
  *ENDPOINT_REGISTER_ADDRESS(ep_num) |= _BV(1);
}

void endpoint_clear_stall(const uint8_t ep_num) {
  *ENDPOINT_REGISTER_ADDRESS(ep_num) &= ~_BV(1);
}

