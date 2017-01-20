#include "usb/remote_renderer.h"
#include "usb/endpoint.h"

#include "frame_buffer.h"
#include "frame_queue.h"

#include <stddef.h>

static struct frame_buffer_t* frame = NULL;
static struct frame_transfer_state_t state;

static void inline clear_frame_state() {
  state.write_pos = NULL;
  state.buffer_end = NULL;
}

static void init_frame_state() {
  if (frame) {
    frame->flags = FRAME_FREE_AFTER_DRAW;
    state.write_pos = frame->buffer;
    state.buffer_end = frame->buffer + get_frame_buffer_size();
  }
  else {
    clear_frame_state();
  }
}

void remote_renderer_init() {
  // If a frame is already allocated, just reset the internal state
  if (!frame) {
    frame = create_frame();
  }

  init_frame_state();
}

void remote_renderer_halt() {
  endpoint_stall(1);
  if (frame) {
    destroy_frame(frame);
    frame = NULL;
  }
  clear_frame_state();
}

void remote_renderer_stop() {
  if (frame) {
    destroy_frame(frame);
    frame = NULL;
  }
  clear_frame_state();
}

struct frame_transfer_state_t* remote_renderer_get_transfer_state() {
  return &state;
}

void remote_renderer_transfer_done() {
  if (push_frame(frame)) {
    frame = create_frame();
    init_frame_state();
  }
  else {
    remote_renderer_halt();
  }
}
