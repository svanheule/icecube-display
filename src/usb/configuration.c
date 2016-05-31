#include "usb/configuration.h"
#include "usb/endpoint.h"
#include <avr/io.h>

// Configuration list
// 64B, single bank, control
static const struct ep_hw_config_t ENDPOINT_0 = {
    0
  , EP_CONTROL
  , EP_BANK_SIZE_64 | EP_BANK_COUNT_1
};

// 64B, single bank, bulk
/*static const struct ep_hw_config_t ENDPOINT_1 = {1, _BV(EPTYPE1), _BV(EPSIZE1)|_BV(EPSIZE0)};*/

// Configuration selection state
static int8_t selected_configuration = -1;

int8_t get_configuration_index() {
  return selected_configuration;
}

bool valid_configuration_index(int8_t index) {
  // FIXME Use runtime determination of largest index
  return (index >= 0) && (index <= 1);
}

bool set_configuration_index(int8_t index) {
  bool cfg_ok = valid_configuration_index(index);
  if (cfg_ok) {
    selected_configuration = index;
    switch(index) {
      case 0:
        cfg_ok = cfg_ok && endpoint_configure(&ENDPOINT_0);
        break;
      case 1:
        cfg_ok = cfg_ok && endpoint_configure(&ENDPOINT_0);
/*        cfg_ok = cfg_ok && endpoint_configure(&ENDPOINT_1);*/
        break;
      default:
        break;
    }
  }
  return cfg_ok;
}
