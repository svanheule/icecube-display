#include "kinetis/io.h"
#include "kinetis/usb_bdt.h"
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENDPOINTS 3

#define BANK_COUNT 1

struct bdt_endpoint_direction_t {
  struct buffer_descriptor_t even;
  struct buffer_descriptor_t odd;
};

struct bdt_endpoint_t {
  struct bdt_endpoint_direction_t rx;
  struct bdt_endpoint_direction_t tx;
};

// BDT needs to be aligned to a 512B boundary (i.e. 9 lower bits of address are 0)
static alignas(512) struct bdt_endpoint_t endpoint_table[MAX_ENDPOINTS];
static struct buffer_descriptor_t* const buffer_descriptor_table =
    (struct buffer_descriptor_t*) &endpoint_table[0];


struct buffer_descriptor_t* get_bdt() {
  return buffer_descriptor_table;
}

// BDT entries are 8 bytes in size, so shift back address bits by 3 positions
struct buffer_descriptor_t* get_buffer_descriptor(uint8_t epnum, uint8_t tx, uint8_t bank) {
  ptrdiff_t index = (epnum << 2) | (tx << 1) | bank;
  return buffer_descriptor_table + index;
}

void clear_bdt() {
  // Reset all endpoints
  memset(&endpoint_table, 0, sizeof(endpoint_table));
}

// Auxiliary functions
#define LSB_MASK(n) ((1<<n)-1)

uint8_t get_token_pid(const struct buffer_descriptor_t* descriptor) {
  return (descriptor->desc >> BDT_DESC_PID) & LSB_MASK(4);
}

uint16_t get_byte_count(const struct buffer_descriptor_t* descriptor) {
  return (descriptor->desc >> BDT_DESC_BC0) & LSB_MASK(10);
}

uint32_t generate_bdt_descriptor(uint16_t length, uint8_t data_toggle) {
  const uint32_t base_desc = _BV(BDT_DESC_OWN) | _BV(BDT_DESC_DTS);
  length &= LSB_MASK(10);
  data_toggle &= LSB_MASK(1);
  return base_desc | (length << BDT_DESC_BC0) | (data_toggle << BDT_DESC_DATA01);
}

// Toggles: bit array of (rx, tx) pairs: {EP0_RX, EP0_TX, EP1_RX, ...}
#define TOGGLE_OFFSET(ep, tx) (((ep) << 1) | (tx))

// Endpoint buffer toggles
static uint32_t buffer_toggles;

void reset_buffer_toggles() {
  uint8_t usb_ctl = USB0_CTL;
  USB0_CTL = usb_ctl | USB_CTL_ODDRST;
  buffer_toggles = 0;
#if BANK_COUNT > 1
  USB0_CTL = usb_ctl & ~USB_CTL_ODDRST;
#endif
}

uint8_t get_buffer_toggle(const uint8_t ep_num, const uint8_t tx) {
#if BANK_COUNT > 1
  return *BITBAND_SRAM_ADDRESS(&buffer_toggles, TOGGLE_OFFSET(ep_num, tx));
#else
  return 0;
#endif
}

uint8_t pop_buffer_toggle(const uint8_t ep_num, const uint8_t tx) {
#if BANK_COUNT > 1
  volatile uint32_t* toggle = BITBAND_SRAM_ADDRESS(&buffer_toggles, TOGGLE_OFFSET(ep_num, tx));
  uint8_t odd = *toggle;
  *toggle = odd ^ 1;
  return odd;
#else
  return 0;
#endif
}


// Endpoint DATA01 toggles
static uint32_t data_toggles;

void reset_data_toggles() {
  data_toggles = 0;
}

uint8_t get_data_toggle(const uint8_t ep_num, const uint8_t tx) {
  return *BITBAND_SRAM_ADDRESS(&data_toggles, TOGGLE_OFFSET(ep_num, tx));
}

void set_data_toggle(const uint8_t ep_num, const uint8_t tx, const uint8_t value) {
  *BITBAND_SRAM_ADDRESS(&data_toggles, TOGGLE_OFFSET(ep_num, tx)) = value;
}

// Endpoint sizes
static uint8_t ep_sizes[MAX_ENDPOINTS];

bool set_endpoint_size(const uint8_t ep_num, const uint8_t ep_size) {
  const bool ep_valid = ep_num < MAX_ENDPOINTS;
  if (ep_valid) {
    ep_sizes[ep_num] = ep_size;
  }
  return ep_valid;
}

uint8_t get_endpoint_size(const uint8_t ep_num) {
  if (ep_num < MAX_ENDPOINTS) {
    return ep_sizes[ep_num];
  }
  else {
    return 0;
  }
}

// Quasi-static endpoint RX buffers
void* ep_buffers[MAX_ENDPOINTS][BANK_COUNT];

bool transfer_mem_alloc(const uint8_t ep_num) {
  const uint8_t ep_size = get_endpoint_size(ep_num);
  uint8_t* buffer = NULL;
  if (ep_size) {
    buffer = malloc(ep_size*BANK_COUNT);
    ep_buffers[ep_num][0] = buffer;
#if BANK_COUNT > 1
    if (buffer && use_double_buffer) {
      ep_buffers[ep_num][1] = buffer + ep_size;
    }
#endif
  }
  return buffer != NULL;
}

void transfer_mem_free(const uint8_t ep_num) {
  if (ep_buffers[ep_num][0]) {
    free(ep_buffers[ep_num][0]);
    ep_buffers[ep_num][0] = NULL;
#if BANK_COUNT > 1
    ep_buffers[ep_num][1] = NULL;
#endif
  }
}

void* get_ep_rx_buffer(const uint8_t ep_num, const uint8_t bank) {
  if (ep_num < MAX_ENDPOINTS && bank < BANK_COUNT) {
    return ep_buffers[ep_num][bank];
  }
  else {
    return NULL;
  }
}
