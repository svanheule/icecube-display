#include "usb/endpoint.h"
#include "kinetis/io.h"
#include "kinetis/usb_bdt.h"

#define ENDPOINT_REGISTER_ADDRESS(i) ((volatile uint8_t*)(&(USB0_ENDPT0) + 4*i))

bool endpoint_configure(const struct ep_config_t* config) {
  endpoint_deconfigure(config->num);
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

  bool config_ok = reg != 0 && set_endpoint_size(config->num, config->size);
  if (config_ok) {
    if (config->dir & EP_DIRECTION_IN) {
      get_buffer_descriptor(config->num, BDT_DIR_TX)->desc = 0;
    }

    if (config->dir & EP_DIRECTION_OUT) {
      // Memory allocation
      config_ok &= transfer_mem_alloc(config->num);
      if (config_ok) {
        struct buffer_descriptor_t* bd = get_buffer_descriptor(config->num, BDT_DIR_RX);
        bd->buffer = get_ep_buffer(config->num);
        // generate_bdt_descriptor() enables data toggle synchronisation by default,
        // which might be undesired behaviour for isochronous endpoints
        bd->desc = generate_bdt_descriptor(config->size, 0);
      }
    }
  }

  if (config_ok && config->init) {
    config->init();
  }

  endpoint_reset_data_toggle(config->num);
  *ENDPOINT_REGISTER_ADDRESS(config->num) = reg;
  return config_ok;
}

void endpoint_deconfigure(const uint8_t ep_num) {
  transfer_mem_free(ep_num);
  set_endpoint_size(ep_num, 0);
  *ENDPOINT_REGISTER_ADDRESS(ep_num) = 0;
}

uint16_t endpoint_get_size(const uint8_t ep_num) {
  return get_endpoint_size(ep_num);
}

bool endpoint_stall(const uint8_t ep_num) {
  volatile uint8_t* ep = ENDPOINT_REGISTER_ADDRESS(ep_num);
  bool can_stall = *ep & (USB_ENDPT_EPRXEN | USB_ENDPT_EPTXEN);
  if (can_stall) {
    *ep |= USB_ENDPT_EPSTALL;
  }
  return can_stall;
}

bool endpoint_clear_stall(const uint8_t ep_num) {
  volatile uint8_t* ep = ENDPOINT_REGISTER_ADDRESS(ep_num);
  bool can_stall = *ep & (USB_ENDPT_EPRXEN | USB_ENDPT_EPTXEN);
  if (can_stall) {
    *ep &= ~USB_ENDPT_EPSTALL;
  }
  return can_stall;
}

bool endpoint_is_stalled(const uint8_t ep_num) {
  volatile uint8_t* ep = ENDPOINT_REGISTER_ADDRESS(ep_num);
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