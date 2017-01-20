#include "usb/configuration.h"
#include "usb/endpoint.h"
#include "remote.h"
#include <string.h>
#include <avr/pgmspace.h>

static const struct ep_config_t CONFIG0_EP_LIST[] PROGMEM = {
  {0, EP_TYPE_CONTROL, EP_DIRECTION_BIDIR, 64, NULL}
};

static const struct ep_config_t CONFIG1_EP_LIST[] PROGMEM = {
    {0, EP_TYPE_CONTROL, EP_DIRECTION_BIDIR, 64, NULL}
  , {1, EP_TYPE_BULK, EP_DIRECTION_OUT, 64, ep1_init}
};

#define CONFIG(config_list) {sizeof(config_list)/sizeof(config_list[0]), &config_list[0]}

// Configuration list
static const struct configuration_t CONFIG_LIST[] PROGMEM = {
    CONFIG(CONFIG0_EP_LIST)
  , CONFIG(CONFIG1_EP_LIST)
};
#define CONFIG_LIST_LENGTH (sizeof(CONFIG_LIST)/sizeof(CONFIG_LIST[0]))

static bool load_configuration(const struct configuration_t* config) {
  uint8_t ep_count = pgm_read_byte(&config->endpoint_count);
  struct ep_config_t* ep_config_list_head;
  memcpy_P(&ep_config_list_head, &config->ep_config_list, sizeof(ep_config_list_head));

  bool config_ok = true;
  struct ep_config_t* ep_config_list_end = ep_config_list_head + ep_count;

  while (ep_config_list_head != ep_config_list_end && config_ok) {
    struct ep_config_t ep_config;
    memcpy_P(&ep_config, ep_config_list_head, sizeof(ep_config));
    config_ok = config_ok && endpoint_configure(&ep_config);
    ep_config_list_head++;
  }
  return config_ok;
}

// Configuration selection state
static int8_t selected_configuration = -1;

int8_t get_configuration_index() {
  return selected_configuration;
}

bool valid_configuration_index(int8_t index) {
  return (index >= 0) && (index < CONFIG_LIST_LENGTH);
}

bool set_configuration_index(int8_t index) {
  bool cfg_ok = valid_configuration_index(index) && load_configuration(&CONFIG_LIST[index]);
  if (cfg_ok) {
    selected_configuration = index;
  }
  return cfg_ok;
}

const struct configuration_t* get_configuration_P(int8_t index) {
  const struct configuration_t* config = NULL;
  if (valid_configuration_index(index)) {
    config = &CONFIG_LIST[index];
  }
  return config;
}
