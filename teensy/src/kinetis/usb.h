#ifndef KINETIS_USB_H
#define KINETIS_USB_H

#include <stdint.h>

#define BDTMEM __attribute__((section(".usbdescriptortable")))

#define BDT_DESC_BC 16
#define BDT_DESC_OWN 7
#define BDT_DESC_DATA01 6
#define BDT_DESC_KEEP 5
#define BDT_DESC_NINC 4
#define BDT_DESC_DTS 3
#define BDT_DESC_STALL 2

struct buffer_descriptor_t {
  uint32_t desc;
  void* buffer;
};

struct bdt_direction_t {
  struct buffer_descriptor_t even;
  struct buffer_descriptor_t odd;
};

#define MAX_TX_BUFFER 2 //(sizeof(struct bdt_direction_t)/sizeof(struct buffer_descriptor_t))

#define BDT_DIR_RX 0
#define BDT_DIR_TX 1

struct bdt_endpoint_t {
  struct bdt_direction_t rx;
  struct bdt_direction_t tx;
};

#endif // KINETIS_USB_H
