#ifndef KINETIS_DMA_H
#define KINETIS_DMA_H

#include "kinetis/io.h"

struct transfer_control_descriptor_t {
  const volatile void* SADDR;
  int16_t SOFF;
  union {
    uint16_t ATTR;
    struct {
      uint8_t ATTR_DST;
      uint8_t ATTR_SRC;
    };
  };
  union {
    uint32_t NBYTES;
    uint32_t NBYTES_MLNO;
    uint32_t NBYTES_MLOFFNO;
    uint32_t NBYTES_MLOFFYES;
  };
  int32_t SLAST;
  volatile void* DADDR;
  int16_t DOFF;
  union {
    uint16_t CITER;
    uint16_t CITER_ELINKYES;
    uint16_t CITER_ELINKNO;
  };
  int32_t DLASTSGA;
  uint16_t CSR;
  union {
    uint16_t BITER;
    uint16_t BITER_ELINKYES;
    uint16_t BITER_ELINKNO;
  };
} __attribute__((packed));

extern volatile struct transfer_control_descriptor_t* const dma_tcd_list;

void clear_channel_tcd(const uint8_t channel);

#endif // KINETIS_DMA_H
