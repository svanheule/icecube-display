#ifndef KINETIS_USB_BDT_H
#define KINETIS_USB_BDT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BDT_DESC_BC0 16
#define BDT_DESC_OWN 7
#define BDT_DESC_DATA01 6
#define BDT_DESC_KEEP 5
#define BDT_DESC_NINC 4
#define BDT_DESC_DTS 3
#define BDT_DESC_STALL 2
#define BDT_DESC_PID 2

struct buffer_descriptor_t {
  uint32_t desc;
  void* buffer;
};

#define BDT_DIR_RX 0
#define BDT_DIR_TX 1

struct buffer_descriptor_t* get_bdt();

void clear_bdt();

struct buffer_descriptor_t* get_buffer_descriptor(uint8_t epnum, uint8_t tx);


uint8_t get_token_pid(const struct buffer_descriptor_t* descriptor);
uint16_t get_byte_count(const struct buffer_descriptor_t* descriptor);

uint32_t generate_bdt_descriptor(uint16_t length, uint8_t data_toggle);

// Ping-pong buffer usage tracking
void reset_buffer_toggles();
uint8_t get_buffer_toggle(const uint8_t ep_num, const uint8_t tx);
uint8_t pop_buffer_toggle(const uint8_t ep_num, const uint8_t tx);

// Data toggle tracking
void reset_data_toggles();
uint8_t get_data_toggle(const uint8_t ep_num, const uint8_t tx);
void set_data_toggle(const uint8_t ep_num, const uint8_t tx, const uint8_t value);

// Transfer memory management
bool transfer_mem_alloc(const uint8_t ep_num, const uint8_t ep_size);
void transfer_mem_free(const uint8_t ep_num);
void* get_ep_buffer(const uint8_t ep_num);

#endif // KINETIS_USB_BDT_H
