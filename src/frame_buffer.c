#include "frame_buffer.h"
#include <math.h>

// Double frame buffer
static unsigned char frame_buffer_0[FRAME_LENGTH];
static unsigned char frame_buffer_1[FRAME_LENGTH];

// TODO Triple buffering with round-robin buffer usage


static unsigned char* current_frame_ptr;
static unsigned char* next_frame_ptr;

static void clear_frame(unsigned char* frame_ptr) {
  unsigned char* frame_end = frame_ptr + FRAME_LENGTH;

  while (frame_ptr != frame_end) {
    *(frame_ptr++) = 0;
  }
}

void init_frame_buffer() {
  clear_frame(frame_buffer_0);
  clear_frame(frame_buffer_1);

  current_frame_ptr = frame_buffer_0;
  next_frame_ptr = frame_buffer_1;
}

const unsigned char *const get_frame_buffer() {
  return current_frame_ptr;
}

