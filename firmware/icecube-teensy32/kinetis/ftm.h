#ifndef KINETIS_FTM_H
#define KINETIS_FTM_H

#include "kinetis/io.h"

struct ftm_channel_config_t {
  uint32_t SC;
  uint32_t VAL;
} __attribute__((packed));

struct ftm_config_t {
  uint32_t SC;
  uint32_t CNT;
  uint32_t MOD;
  struct ftm_channel_config_t channels[8];
  uint32_t CNTIN;
  uint32_t STATUS;
  uint32_t MODE;
  uint32_t SYNC;
  uint32_t OUTINIT;
  uint32_t OUTMASK;
  uint32_t COMBINE;
  uint32_t DEADTIME;
  uint32_t EXTTRIG;
  uint32_t POL;
  uint32_t FMS;
  uint32_t FILTER;
  uint32_t FLTCTRL;
  uint32_t QDCTRL;
  uint32_t CONF;
  uint32_t FLTPOL;
  uint32_t SYNCONF;
  uint32_t INVCTRL;
  uint32_t SWOCTRL;
  uint32_t PWMLOAD;
} __attribute__((packed));

extern volatile struct ftm_config_t* const ftm0_config;
extern volatile struct ftm_config_t* const ftm1_config;
extern volatile struct ftm_config_t* const ftm2_config;

#endif // KINETIS_FTM_H
