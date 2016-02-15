#ifndef DISPLAY_STATE_H
#define DISPLAY_STATE_H

#include <stdint.h>

enum display_state_t {
    DISPLAY_IDLE
  , DISPLAY_EXTERNAL
  , DISPLAY_DEMO
  , DISPLAY_TEST_RING
  , DISPLAY_TEST_SNAKE
};

enum display_command_t {
    DISPLAY_GOTO_IDLE
  , DISPLAY_GOTO_EXTERNAL
  , DISPLAY_GOTO_DEMO
  , DISPLAY_GOTO_TEST_RING
  , DISPLAY_GOTO_TEST_SNAKE
};

struct renderer_t {
  void (*start)();
  void (*stop)();
  struct frame_buffer_t (*render_frame)();
};

enum display_state_t get_display_state();

void advance_display_state(enum display_command_t command);

#endif // DISPLAY_STATE_H
