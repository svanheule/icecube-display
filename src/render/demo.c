#include "render/demo.h"
#include "stdlib.h"
#include "switches.h"
#include <avr/pgmspace.h>

static void init_demo();
static void stop_demo();
static struct frame_buffer_t* render_demo();

static const struct renderer_t DEMO_RENDERER = {
    init_demo
  , stop_demo
  , render_demo
};

const struct renderer_t* get_demo_renderer() {
  return &DEMO_RENDERER;
}

struct pulse_t {
  uint16_t time; //< Turn-on time of led
  uint8_t led_index;
  struct led_t led; //< LED brightness and colour
};

struct event_t {
  const struct pulse_t* pulses_start; //< Array of pulses
  const struct pulse_t* pulses_end; //< Past-the-end pointer
};

// Macros to define external symbols
#define BIN_START(name) _binary_## name ## _start
#define BIN_END(name) _binary_ ## name ## _end
#define BIN_SIZE_SYM(name) _binary_ ## name ## _size
#define BIN_SIZE(name) ((uint16_t) BIN_SIZE_SYM(name))

#define EXTERNAL_EVENT(name) \
extern const struct pulse_t BIN_START(name)[] PROGMEM;\
extern const struct pulse_t BIN_END(name)[] PROGMEM;\
extern const uint16_t BIN_SIZE_SYM(name)[];

// Create list (stored in PROGMEM)

EXTERNAL_EVENT(event_s125_0_0_5_zen_0_0_1_bin)
EXTERNAL_EVENT(event_s125_1_1_5_zen_0_0_1_bin)
EXTERNAL_EVENT(event_s125_2_2_5_zen_0_0_1_bin)
EXTERNAL_EVENT(event_s125_0_0_5_zen_0_2_0_25_bin)
EXTERNAL_EVENT(event_s125_1_1_5_zen_0_2_0_25_bin)
EXTERNAL_EVENT(event_s125_2_2_5_zen_0_2_0_25_bin)
EXTERNAL_EVENT(event_s125_0_0_5_zen_0_3_0_35_bin)
EXTERNAL_EVENT(event_s125_1_1_5_zen_0_3_0_35_bin)
EXTERNAL_EVENT(event_s125_2_2_5_zen_0_3_0_35_bin)

#define EVENT_ITEM(name) {BIN_START(name), BIN_END(name)}

static const struct event_t events[] PROGMEM = {
    EVENT_ITEM(event_s125_0_0_5_zen_0_0_1_bin)
  , EVENT_ITEM(event_s125_1_1_5_zen_0_0_1_bin)
  , EVENT_ITEM(event_s125_2_2_5_zen_0_0_1_bin)
  , EVENT_ITEM(event_s125_0_0_5_zen_0_2_0_25_bin)
  , EVENT_ITEM(event_s125_1_1_5_zen_0_2_0_25_bin)
  , EVENT_ITEM(event_s125_2_2_5_zen_0_2_0_25_bin)
  , EVENT_ITEM(event_s125_0_0_5_zen_0_3_0_35_bin)
  , EVENT_ITEM(event_s125_1_1_5_zen_0_3_0_35_bin)
  , EVENT_ITEM(event_s125_2_2_5_zen_0_3_0_35_bin)
};
static const struct event_t* events_end = events + sizeof(events)/sizeof(struct event_t);


// Constants in number of frames (25 fps)
//! On time of an LED for each pulse.
static const uint8_t PULSE_DURATION = 25;
//! Off time of the entire display after all pulses.
static const uint8_t PULSE_CLEAR_DURATION = 33;
//! Duration of the event overview.
static const uint8_t OVERVIEW_DURATION = 75;
//! Off time of the entire display after the event overview.
static const uint8_t OVERVIEW_CLEAR_DURATION = 50;

// Module variables
static const struct event_t* current_event;
static const struct pulse_t* current_pulse;
static const struct pulse_t* pulses_end;

static uint16_t frame_number;
static uint16_t mode_end;

enum render_mode_t {
    TIME_LAPSE
  , TIME_LAPSE_CLEAR
  , OVERVIEW
  , OVERVIEW_CLEAR
};

static bool paused;

static enum render_mode_t render_mode;

static void reset_event_P(const struct event_t* event) {
  current_pulse = (const struct pulse_t*) pgm_read_word(&(event->pulses_start));
}

static void load_event_P(const struct event_t* event) {
  render_mode = TIME_LAPSE;
  frame_number = 0;
  mode_end = 0;

  pulses_end = (const struct pulse_t*) pgm_read_word(&(event->pulses_end));
  reset_event_P(event);
}

static void load_next_event() {
  // Go to next event if the current isn't the last, otherwise reset
  ++current_event;
  if (current_event == events_end) {
    current_event = &(events[0]);
  }
  load_event_P(current_event);
}

static void init_demo() {
  clear_switch_pressed(SWITCH_PLAY_PAUSE);
  clear_switch_pressed(SWITCH_FORWARD);
  paused = false;
  current_event = &events[0];
  load_event_P(current_event);
}

static void stop_demo() {
  current_event = 0;
}

static struct frame_buffer_t* render_demo() {
  struct frame_buffer_t* frame = 0;
  // Only allocate a frame if there's something to be rendered
  if (current_event) {
    frame = create_empty_frame();
  }

  if (frame) {
    // Allow frame to be released
    frame->flags = FRAME_FREE_AFTER_DRAW;

    // Loop over currently shown pulses
    const struct pulse_t* pulse = current_pulse;
    while ( pulse != pulses_end
        && (paused || pgm_read_word(&(pulse->time)) <= frame_number)
    ) {
      uint8_t index = pgm_read_byte(&(pulse->led_index));
      memcpy_P(&(frame->buffer[index]), &(pulse->led), sizeof(struct led_t));
      ++pulse;
    }

    // Check if the pause switch was pressed
    if (switch_pressed(SWITCH_PLAY_PAUSE)) {
      clear_switch_pressed(SWITCH_PLAY_PAUSE);
      paused = !paused;
      load_event_P(current_event);
    }

    // Check if current starting pulse or rendering mode should be changed
    if (switch_pressed(SWITCH_FORWARD)) {
      clear_switch_pressed(SWITCH_FORWARD);
      load_next_event();
    }
    else if (!paused) {
      switch (render_mode) {
        case TIME_LAPSE:
          if ( current_pulse != pulses_end
            && pgm_read_word(&(current_pulse->time))+PULSE_DURATION <= frame_number
          ) {
            ++current_pulse;
            // If last pulse was reached, set display clear time-out and change mode
            if (current_pulse == pulses_end) {
              render_mode = TIME_LAPSE_CLEAR;
              mode_end = frame_number + PULSE_CLEAR_DURATION;
            }
          }
          break;
        case TIME_LAPSE_CLEAR:
          if (frame_number == mode_end) {
            render_mode = OVERVIEW;
            mode_end = frame_number + OVERVIEW_DURATION;
            // Reset pulse pointer to first pulse
            // This will cause the renderer to show all pulses in next frame
            reset_event_P(current_event);
          }
          break;
        case OVERVIEW:
          if (frame_number == mode_end) {
            render_mode = OVERVIEW_CLEAR;
            // Set pulse pointer to point past the end so no pulses are shown in the next frame
            current_pulse = pulses_end;
            mode_end = frame_number + OVERVIEW_CLEAR_DURATION;
          }
          break;
        case OVERVIEW_CLEAR:
          if (frame_number == mode_end) {
            load_next_event();
          }
          break;
        default:
          break;
      }
    }

    ++frame_number;
  }


  return frame;
}
