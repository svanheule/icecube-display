#include "demo.h"
#include <avr/pgmspace.h>

struct event_t {
  const struct pulse_t* pulses_start; //< Array of pulses
  const struct pulse_t* pulses_end; //< Past-the-end pointer
};

struct item_t {
  struct event_t event;
  struct item_t* next_item;
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


// Module global variables
static const struct event_t* current_event;
static const struct pulse_t* current_pulse;
static const struct pulse_t* pulses_end;

static uint16_t frame_number;
static uint8_t led_remaining_on[LED_COUNT];
static uint16_t last_frame_number;

enum render_mode_t {
    TIME_LAPSE
  , OVERVIEW_MODE
};

static enum render_mode_t render_mode;

static void reset_event_P(const struct event_t* event) {
  current_pulse = (struct pulse_t*) pgm_read_word(&(event->pulses_start));
}

static void load_event_P(const struct event_t* event) {
  frame_number = 0;
  last_frame_number = 0;

  pulses_end = (struct pulse_t*) pgm_read_word(&(event->pulses_end));
  reset_event_P(event);
}

void init_demo() {
  render_mode = TIME_LAPSE;
  current_event = &events[0];
  load_event_P(current_event);
  int i = LED_COUNT-1;
  do {
    led_remaining_on[i] = 0;
  } while(i--);
}

uint8_t demo_finished() {
  return current_event ? 0 : 1;
}

void render_demo(frame_t* buffer) {
  if (frame_number == 0) {
    clear_frame(buffer);
  }

  if (current_event) {
    // Decrement on-times and turn off timed out LEDs
    uint8_t i;
    for (i = 0; i < LED_COUNT; ++i) {
      if (led_remaining_on[i] > 0) {
        // Decrease remaining on-time
        --led_remaining_on[i];
        if (led_remaining_on[i] == 0) {
          (*buffer)[i] = (struct led_t) {0, 0, 0, 0};
        }
      }
    }

    // Set frame_number to its next value
    ++frame_number;

    if (render_mode == TIME_LAPSE) {
      // Read newly fired pulses
      while (current_pulse != pulses_end && pgm_read_word(&(current_pulse->time)) < frame_number) {
        // Load pulse from PROGMEM
        struct pulse_t pulse;
        memcpy_P(&pulse, current_pulse, sizeof(struct pulse_t));
        // Set LED to its given colour and advance pointer
        (*buffer)[pulse.led_index] = pulse.led;
        led_remaining_on[pulse.led_index] = PULSE_DURATION;
        last_frame_number = frame_number + PULSE_DURATION;
        ++current_pulse;
        if (current_pulse == pulses_end) {
          last_frame_number += PULSE_CLEAR_DURATION;
        }
      }
    }
    else if (render_mode == OVERVIEW_MODE && current_pulse != pulses_end) {
//      current_pulse = (struct pulse_t*) pgm_read_word(&(current_event->pulses_start));
      // Load all pulses from PROGMEM
      struct pulse_t pulse;
      while (current_pulse != pulses_end) {
        memcpy_P(&pulse, current_pulse, sizeof(struct pulse_t));
        // Set LED to its given colour and advance pointer
        (*buffer)[pulse.led_index] = pulse.led;
        led_remaining_on[pulse.led_index] = OVERVIEW_DURATION;
        ++current_pulse;
      }
    }

    // Determination of event display ending
    if (frame_number == last_frame_number && current_pulse == pulses_end) {
      // If in TIME_LAPSE mode, first proceed to OVERVIEW_MODE
      switch (render_mode) {
        case TIME_LAPSE:
          render_mode = OVERVIEW_MODE;
          reset_event_P(current_event);
          last_frame_number += OVERVIEW_DURATION + OVERVIEW_CLEAR_DURATION;
          break;

        case OVERVIEW_MODE:
          // Proceed to next list item
          render_mode = TIME_LAPSE;
          ++current_event;
          if (current_event != events_end) {
            load_event_P(current_event);
          }
          else {
            current_event = 0;
          }
          break;
      }
    }
  }
  else {
    clear_frame(buffer);
  }
}
