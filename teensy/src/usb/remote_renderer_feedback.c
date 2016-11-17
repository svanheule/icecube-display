#include "usb/remote_renderer_feedback.h"
#include "kinetis/usb_bdt.h"
#include "frame_timer.h"

static struct display_frame_usb_phase_t data = {0, 0xffff};

void init_remote_renderer_feedback() {
  struct buffer_descriptor_t* bd = get_buffer_descriptor(2, BDT_DIR_TX);
  bd->buffer = &data;
  bd->desc = generate_bdt_descriptor(sizeof(data), 0);
  set_data_toggle(2, BDT_DIR_TX, 1);
}
