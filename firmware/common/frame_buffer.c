#include "frame_buffer.h"
#include <util/atomic.h>
#include <stdlib.h>
#include <string.h>

// List of statically allocated frame buffers
#define BUFFER_LIST_LENGTH 3
static struct frame_buffer_t buffer_list[BUFFER_LIST_LENGTH];

// Dynamically allocated buffer memory
static uint8_t* buffer_data;

// Mask indicating which buffers are already handed out
static uint8_t buffer_taken;

size_t get_display_buffer_size() {
  return get_led_size()*get_led_count();
}

void init_display_buffers() {
  const size_t buffer_size = get_display_buffer_size();
  buffer_data = malloc(buffer_size*BUFFER_LIST_LENGTH);
  if (buffer_data) {
    for (uint8_t i = 0; i < BUFFER_LIST_LENGTH; ++i) {
      buffer_list[i].buffer = buffer_data + i*buffer_size;
    }
  }
}

// Frame memory management functions
struct frame_buffer_t* create_frame() {
  struct frame_buffer_t* f = 0;
  uint8_t buffer = BUFFER_LIST_LENGTH-1;
  uint8_t buffer_mask = 1<<(BUFFER_LIST_LENGTH-1);
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    /* Look for available buffer.
     * Start by looking at the last buffer and shift the bit mask to the right.
     * If no free buffer has been encountered by then (i.e. bit set to '0' in buffer_taken)
     * buffer_mask will become zero and loop evaluation will also end.
     */
    while (buffer_mask & buffer_taken) {
      --buffer;
      buffer_mask >>= 1;
    }
    // Return free buffer, if any. Reuse bit mask from loop to mark buffer as taken.
    if (buffer_mask) {
      buffer_taken |= buffer_mask;
      f = &(buffer_list[buffer]);
    }
  }
  return f;
}

void destroy_frame(struct frame_buffer_t* frame) {
  struct frame_buffer_t* buffer = buffer_list + (BUFFER_LIST_LENGTH-1);
  uint8_t buffer_mask = 1<<(BUFFER_LIST_LENGTH-1);
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    // Compare pointer to list of allocated buffers
    // See create_frame() for details on the variable manipulations.
    while (frame != buffer && buffer_mask) {
      --buffer;
      buffer_mask >>= 1;
    }
    // If found, mark as free again
    if (buffer_mask) {
      buffer_taken &= ~buffer_mask;
    }
  }
}

// Commom frame operations
void clear_frame(struct frame_buffer_t* frame_ptr) {
  if (frame_ptr) {
    memset(frame_ptr->buffer, 0, get_display_buffer_size());
  }
}


struct frame_buffer_t* create_empty_frame() {
  struct frame_buffer_t* f = create_frame();
  if (f) {
    clear_frame(f);
  }
  return f;
}
