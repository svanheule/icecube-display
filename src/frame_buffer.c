#include "frame_buffer.h"

void clear_frame(frame_t* frame_ptr) {
  struct led_t* write_ptr = *frame_ptr;
  struct led_t* frame_end = write_ptr + LED_COUNT;

  while (write_ptr != frame_end) {
    *(write_ptr++) = (struct led_t) {0, 0, 0, 0};
  }
}

// Definitions of frame FIFO
#include <stdlib.h>

#define QUEUE_SIZE 2
static struct frame_buffer_t* frame_queue[QUEUE_SIZE];
static volatile uint8_t head;
static volatile bool head_wrapped;
static volatile uint8_t tail;
static volatile bool tail_wrapped;

struct frame_buffer_t* create_frame() {
  return (struct frame_buffer_t*) malloc(sizeof(struct frame_buffer_t));
}

bool frame_queue_full() {
  return (head == tail) && (tail_wrapped != head_wrapped);
}

bool frame_queue_empty() {
  return (head == tail) && (tail_wrapped == head_wrapped);
}

bool push_frame(struct frame_buffer_t* frame) {
  if (!frame_queue_full()) {
    frame_queue[head] = frame;
    ++head;
    if (head == QUEUE_SIZE) {
      head = 0;
      head_wrapped = !head_wrapped;
    }
    return true;
  }
  else {
    return false;
  }
}

struct frame_buffer_t* pop_frame() {
  if (!frame_queue_empty()) {
    struct frame_buffer_t* frame = frame_queue[tail];
    ++tail;
    if (tail == QUEUE_SIZE) {
      tail = 0;
      tail_wrapped = !tail_wrapped;
    }
    return frame;
  }
  else {
    return (struct frame_buffer_t*) 0;
  }
}
