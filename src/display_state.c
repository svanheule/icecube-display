#include "display_state.h"
#include "frame_buffer.h"

static enum display_state_t display_state = DISPLAY_IDLE;

void advance_display_state(enum display_command_t command) {
  switch (display_state) {

    case DISPLAY_IDLE:
      switch (command) {
        case DISPLAY_GOTO_EXTERNAL:
          display_state = DISPLAY_EXTERNAL;
          break;
        case DISPLAY_GOTO_DEMO:
          display_state = DISPLAY_DEMO;
          break;
        case DISPLAY_GOTO_TEST_RING:
          display_state = DISPLAY_TEST_RING;
          break;
        case DISPLAY_GOTO_TEST_SNAKE:
          display_state = DISPLAY_TEST_SNAKE;
          break;
        default:
          break;
        }
      break;

    case DISPLAY_DEMO:
    case DISPLAY_TEST_RING:
    case DISPLAY_TEST_SNAKE:
    case DISPLAY_EXTERNAL:
      if (command == DISPLAY_GOTO_IDLE) {
        display_state = DISPLAY_IDLE;
      }
      break;

    // In case the state machine is in an undefined state, return to DISPLAY_IDLE.
    // This should not happen, but you never know...
    default:
      display_state = DISPLAY_IDLE;
      break;
  }
}


enum display_state_t get_display_state() {
  return display_state;
}
