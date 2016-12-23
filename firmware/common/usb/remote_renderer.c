#include "usb/remote_renderer.h"
#include "usb/endpoint.h"

#include "frame_buffer.h"
#include "frame_queue.h"

#include <stddef.h>

static struct frame_buffer_t* frame = NULL;
static struct remote_transfer_t remote_transfer;

static void inline init_state() {
  if (frame) {
    frame->flags = FRAME_FREE_AFTER_DRAW;
    remote_transfer.buffer_remaining = get_frame_buffer_size();
    remote_transfer.buffer_pos = frame->buffer;
  }
  else {
    remote_transfer.buffer_remaining = 0;
    remote_transfer.buffer_pos = NULL;
  }
}

void remote_renderer_stop() {
  destroy_frame(frame);
  frame = NULL;
  remote_transfer.buffer_remaining = 0;
  remote_transfer.buffer_pos = NULL;
}

struct remote_transfer_t* remote_renderer_get_current() {
  if (!remote_transfer.buffer_pos) {
    if (!frame) {
      frame = create_frame();
    }
    init_state();
  }

  if (remote_transfer.buffer_pos) {
    return &remote_transfer;
  }
  else {
    return NULL;
  }
}

bool remote_renderer_finish() {
  bool finished = remote_transfer.buffer_remaining == 0;

  if (finished && push_frame(frame)) {
    frame = create_frame();
  }

  init_state();
  return finished;
}
