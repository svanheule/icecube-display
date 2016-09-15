#ifndef KINETIS_PIT_H
#define KINETIS_PIT_H

#include <stdint.h>

void enable_pit_module();

struct pit_channel_config_t {
  uint32_t LDVAL;
  const uint32_t CVAL;
  uint32_t TCTRL;
  uint32_t TFLG;
};

extern volatile struct pit_channel_config_t* const pit_channels;

#endif // KINETIS_PIT_H
