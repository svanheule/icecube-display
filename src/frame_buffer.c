#include "frame_buffer.h"
#include <stdlib.h>
#include <util/atomic.h>

struct frame_buffer_t* create_frame() {
  struct frame_buffer_t* f;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    f = (struct frame_buffer_t*) malloc(sizeof(struct frame_buffer_t));
  }
  return f;
}

void destroy_frame(struct frame_buffer_t* frame) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    free(frame);
  }
}

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
