#include "demo.h"
#include <avr/pgmspace.h>

struct item_t {
  struct event_t event;
  struct item_t* next_item;
};

// Macros to define external symbols
#define BIN_START(name) _binary_## name ## _start
#define BIN_END(name) _binary_ ## name ## _end
#define BIN_SIZE(name) _binary_ ## name ## _size

#define EXTERNAL_EVENT(name) \
extern const struct pulse_t BIN_START(name) PROGMEM; \
extern const struct pulse_t BIN_END(name) PROGMEM;

EXTERNAL_EVENT(___event_bin)

// Create forward linked list (stored in PROGMEM)
static struct item_t event_0;

// Module global variables
static const struct item_t* current_item;
static const struct pulse_t* current_pulse;
static const struct pulse_t* pulses_end;

static uint16_t frame_number;
static uint8_t led_remaining_on[LED_COUNT];
static uint16_t last_frame_number;

static void load_event(const struct item_t* item) {
  frame_number = 0;
  last_frame_number = 0;

  current_pulse = item->event.pulses;
  pulses_end = current_pulse + item->event.length;
}

void init_demo() {
  event_0.event.pulses = &BIN_START(___event_bin);
  event_0.event.length = &BIN_END(___event_bin) - &BIN_START(___event_bin);

  current_item = &event_0;
  load_event(&event_0);
  int i = LED_COUNT-1;
  do {
    led_remaining_on[i] = 0;
  } while(i--);
}

uint8_t demo_finished() {
  return current_item ? 0 : 1;
}

void render_demo(frame_t* buffer) {
  if (frame_number == 0) {
    clear_frame(buffer);
  }

  if (current_item) {
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
        last_frame_number += CLEAR_DURATION;
      }
    }

    // Determination of event display ending
    if (frame_number == last_frame_number && current_pulse == pulses_end) {
      // Proceed to next list item
      current_item = current_item->next_item;
      if (current_item) {
        load_event(current_item);
      }
    }
  }
  else {
    clear_frame(buffer);
  }
}
