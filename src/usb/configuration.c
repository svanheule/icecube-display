#include "usb/configuration.h"
#include "usb/endpoint.h"

struct configuration_t {
  uint8_t endpoint_count;
  const struct ep_config_t* ep_config_list;
};

static const struct ep_config_t EP_CONFIG_LIST[1] = {
  {0, EP_TYPE_CONTROL, EP_DIRECTION_BIDIR, 64}
};

// Configuration list
static const struct configuration_t DEFAULT_CONFIG = {1, &EP_CONFIG_LIST[0]};

static bool load_configuration(const struct configuration_t* config) {
  bool config_ok = true;
  uint8_t endpoint = 0;
  while (endpoint < config->endpoint_count && config_ok) {
    config_ok = config_ok && endpoint_configure(&config->ep_config_list[endpoint]);
    ++endpoint;
  }
  return config_ok;
}

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
      case 1:
        load_configuration(&DEFAULT_CONFIG);
        break;
      default:
        break;
    }
  }
  return cfg_ok;
}
