#include "kinetis/io.h"
#include "kinetis/usb_bdt.h"
#include <stdalign.h>
#include <string.h>

// Currently only EP0
#define MAX_ENDPOINTS 1

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
struct buffer_descriptor_t* get_buffer_descriptor(uint8_t epnum, uint8_t tx, uint8_t odd) {
  ptrdiff_t index = (epnum << 2) | (tx << 1) | odd;
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
  return (descriptor->desc >> BDT_DESC_BC) & LSB_MASK(10);
}

uint32_t generate_bdt_descriptor(uint16_t length, uint8_t data_toggle) {
  const uint32_t base_desc = _BV(BDT_DESC_OWN) | _BV(BDT_DESC_DTS);
  length &= LSB_MASK(10);
  data_toggle &= LSB_MASK(1);
  return base_desc | (length << BDT_DESC_BC) | (data_toggle << BDT_DESC_DATA01);
}
