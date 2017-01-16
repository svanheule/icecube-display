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

struct buffer_descriptor_t* get_buffer_descriptor(uint8_t epnum, uint8_t tx, uint8_t bank);


uint8_t get_token_pid(const struct buffer_descriptor_t* descriptor);
uint16_t get_byte_count(const struct buffer_descriptor_t* descriptor);

uint32_t generate_bdt_descriptor(uint16_t length, uint8_t data_toggle);

uint8_t get_buffer_bank_count();

// Ping-pong buffer usage tracking
void reset_buffer_toggles();
uint8_t get_buffer_toggle(const uint8_t ep_num, const uint8_t tx);
uint8_t pop_buffer_toggle(const uint8_t ep_num, const uint8_t tx);


// Ping-pong buffer RX queue
// (push, dequeue) <-> [IN] [queue] [OUT] -> (pop)
// *FUNCTIONS ARE NOT THREAD SAFE*;
/// Increase the queue counter.
/// \todo Enable queueing of any buffer
/// \returns `false` if the queue was full
bool ep_rx_buffer_push(const uint8_t ep_num);
/// Decrease the queue counter and pop a buffer toggle to keep in sync with hardware.
/// \returns `false` if the buffer was empty.
/// \returns The buffer at the front of the queue or NULL if the queue was empty.
bool ep_rx_buffer_pop(const uint8_t ep_num);
/// Decrease the queue counter without popping a buffer toggle.
/// This modifies BDT entries whose OWN bit is set. Use only when the endpoint is halted!
/// \returns `false` if queue was empty.
bool ep_rx_buffer_dequeue(const uint8_t ep_num);
void ep_rx_buffer_dequeue_all(const uint8_t ep_num);

// Data toggle tracking
void reset_data_toggles();
uint8_t get_data_toggle(const uint8_t ep_num, const uint8_t tx);
void set_data_toggle(const uint8_t ep_num, const uint8_t tx, const uint8_t value);
uint8_t pop_data_toggle(const uint8_t ep_num, const uint8_t tx);

// Endpoint size tracking
bool set_endpoint_size(const uint8_t ep_num, const uint8_t ep_size);
uint8_t get_endpoint_size(const uint8_t ep_num);

// Transfer memory management
bool transfer_mem_alloc(const uint8_t ep_num);
void transfer_mem_free(const uint8_t ep_num);
void* get_ep_rx_buffer(const uint8_t ep_num, const uint8_t bank);

#endif // KINETIS_USB_BDT_H
