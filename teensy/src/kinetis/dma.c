#include "kinetis/dma.h"

volatile struct transfer_control_descriptor_t* const dma_tcd_list =
    (volatile struct transfer_control_descriptor_t* const) 0x40009000;

void clear_channel_tcd(const uint8_t channel) {
  dma_tcd_list[channel] = (struct transfer_control_descriptor_t) {
    .SADDR=0,
    .SOFF=0,
    .ATTR=0,
    .NBYTES=0,
    .SLAST=0,
    .DADDR=0,
    .DOFF=0,
    .CITER=0,
    .DLASTSGA=0,
    .CSR=0,
    .BITER=0
  };
}

