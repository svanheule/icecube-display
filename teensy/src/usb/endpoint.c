#include "usb/endpoint.h"
#include "kinetis/io.h"
#include "kinetis/usb_bdt.h"

#define ENDPOINT_REGISTER_ADDRESS(i) ((volatile uint8_t*)(&(USB0_ENDPT0) + 4*i))

bool endpoint_configure(const struct ep_config_t* config) {
  uint8_t reg = 0;

  if (config->type == EP_TYPE_CONTROL && config->dir == EP_DIRECTION_BIDIR) {
    reg = USB_ENDPT_EPTXEN | USB_ENDPT_EPRXEN | USB_ENDPT_EPHSHK;
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

  // Memory is currently allocated in remote_usb.c

  endpoint_reset_data_toggle(config->num);
  *ENDPOINT_REGISTER_ADDRESS(config->num) = reg;
  return reg != 0;
}

void endpoint_deconfigure(const uint8_t ep_num) {
  *ENDPOINT_REGISTER_ADDRESS(ep_num) = 0;
}

bool endpoint_stall(const uint8_t ep_num) {
  uint8_t* ep = ENDPOINT_REGISTER_ADDRESS(ep_num);
  bool can_stall = *ep & (USB_ENDPT_EPRXEN | USB_ENDPT_EPTXEN);
  if (can_stall) {
    *ep |= USB_ENDPT_EPSTALL;
  }
  return can_stall;
}

bool endpoint_clear_stall(const uint8_t ep_num) {
  uint8_t* ep = ENDPOINT_REGISTER_ADDRESS(ep_num);
  bool can_stall = *ep & (USB_ENDPT_EPRXEN | USB_ENDPT_EPTXEN);
  if (can_stall) {
    *ep &= ~USB_ENDPT_EPSTALL;
  }
  return can_stall;
}

bool endpoint_is_stalled(const uint8_t ep_num) {
  uint8_t* ep = ENDPOINT_REGISTER_ADDRESS(ep_num);
  bool stalled = (*ep & (USB_ENDPT_EPRXEN | USB_ENDPT_EPTXEN)) && (*ep & USB_ENDPT_EPSTALL);
  return stalled;
}

// Data toggle functions
void endpoint_reset_data_toggle(const uint8_t ep_num) {
  volatile uint8_t* ep = ENDPOINT_REGISTER_ADDRESS(ep_num);
  // Check RX first so data toggle reset applies to RX for control endpoints
  set_data_toggle(ep_num, (*ep & USB_ENDPT_EPRXEN) ? 0 : 1, 0);
}

uint8_t endpoint_get_data_toggle(const uint8_t ep_num) {
  volatile uint8_t* ep = ENDPOINT_REGISTER_ADDRESS(ep_num);
  // Check RX first so data toggle get applies to RX for control endpoints
  return get_data_toggle(ep_num, (*ep & USB_ENDPT_EPRXEN) ? 0 : 1);
}
