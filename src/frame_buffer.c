#include "frame_buffer.h"
#include <stdlib.h>
#include <util/atomic.h>

// List of statically allocated frame buffers
#define BUFFER_LIST_LENGTH 3
static struct frame_buffer_t buffer_list[BUFFER_LIST_LENGTH];

// Mask indicating which buffers are already handed out
static uint8_t buffer_taken;

static inline bool is_buffer_free(uint8_t buffer) {
  return !(buffer_taken & (1<<buffer));
}

static inline void mark_buffer_taken(uint8_t buffer) {
  buffer_taken |= (1<<buffer);
}

static inline void mark_buffer_free(uint8_t buffer) {
  buffer_taken &= ~(1<<buffer);
}

// Frame memory management functions
struct frame_buffer_t* create_frame() {
  struct frame_buffer_t* f = 0;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    // Look for free available buffer
    uint8_t buffer = 0;
    while (buffer < BUFFER_LIST_LENGTH && !is_buffer_free(buffer)) {
      ++buffer;
    }
    // Return free buffer, if any
    if (buffer != BUFFER_LIST_LENGTH) {
      mark_buffer_taken(buffer);
      f = &(buffer_list[buffer]);
    }
  }
  return f;
}

void destroy_frame(struct frame_buffer_t* frame) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    // Compare pointer to list of allocated buffers
    uint8_t buffer = 0;
    while (buffer < BUFFER_LIST_LENGTH && frame != &buffer_list[buffer]) {
      ++buffer;
    }
    // If found, mark as free again
    if (buffer != BUFFER_LIST_LENGTH) {
      mark_buffer_free(buffer);
    }
  }
}

// Commom frame operations
void clear_frame(struct frame_buffer_t* frame_ptr) {
  if (frame_ptr) {
    struct led_t* write_ptr = frame_ptr->buffer;
    struct led_t* frame_end = write_ptr + LED_COUNT;

    while (write_ptr != frame_end) {
      *(write_ptr++) = (struct led_t) {0, 0, 0, 0};
    }
  }
}


struct frame_buffer_t* create_empty_frame() {
  struct frame_buffer_t* f = create_frame();
  if (f) {
    clear_frame(f);
  }
  return f;
}
