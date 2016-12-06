#include "frame_queue.h"
#include <stdint.h>
#include <util/atomic.h>

#define QUEUE_SIZE 2
static struct frame_buffer_t* frame_queue[QUEUE_SIZE];
static uint8_t write;
static bool write_wrapped;
static uint8_t read;
static bool read_wrapped;

bool frame_queue_full() {
  return (write == read) && (read_wrapped != write_wrapped);
}

bool frame_queue_empty() {
  return (write == read) && (read_wrapped == write_wrapped);
}

bool push_frame(struct frame_buffer_t* frame) {
  bool can_push = false;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    can_push = !frame_queue_full();
    if (can_push) {
      frame_queue[write] = frame;
      write = (write+1)%QUEUE_SIZE;
      if (write == 0) {
        write_wrapped = !write_wrapped;
      }
    }
  }
  return can_push;
}

struct frame_buffer_t* pop_frame() {
  struct frame_buffer_t* ptr = 0;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (!frame_queue_empty()) {
      ptr = frame_queue[read];
      read = (read+1)%QUEUE_SIZE;
      if (read == 0) {
        read_wrapped = !read_wrapped;
      }
    }
  }
  return ptr;
}
