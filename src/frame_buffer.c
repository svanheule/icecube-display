#include "frame_buffer.h"
#include <math.h>
#include <util/atomic.h>

// Double frame buffer
// Array of arrays of led_t objects
static frame_t frame_buffers[2];

// Pointers to front and back buffers
static frame_t* front_ptr = frame_buffers;
static frame_t* back_ptr = frame_buffers+1;


frame_t* get_front_buffer() {
  return front_ptr;
}


frame_t* get_back_buffer() {
  return back_ptr;
}


void flip_pages() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    frame_t* tmp = front_ptr;
    front_ptr = back_ptr;
    back_ptr = tmp;
  }
}


void clear_frame(frame_t* frame_ptr) {
  struct led_t* write_ptr = *frame_ptr;
  struct led_t* frame_end = write_ptr + LED_COUNT;

  while (write_ptr != frame_end) {
    *(write_ptr++) = (struct led_t) {0, 0, 0, 0};
  }
}

