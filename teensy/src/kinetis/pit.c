#include "kinetis/io.h"

volatile struct pit_channel_config_t* const pit_channels =
  (volatile struct pit_channel_config_t*) 0x40037100;

void enable_pit_module() {
  ATOMIC_REGISTER_BIT_SET(SIM_SCGC6, 23); // enable PIT module clock
  PIT_MCR = 0; // enable PIT module
}
