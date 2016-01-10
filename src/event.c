#include "event.h"

static const struct pulse_t* current_pulse;
static const struct pulse_t* pulses_end;
static uint8_t frame_number;
static uint8_t led_remaining_on[LED_COUNT];

static void render_frame(frame_t* buffer) {

  // Decrement on-times and turn of timed out LEDs
  uint8_t i;
  for (i = 0; i < LED_COUNT; ++i) {
    if (led_remaining_on[i] > 0) {
      // Decrease remaining on-time
      --led_remaining_on[i];
      // If no more time remains, turn the LED off
      if (!led_remaining_on[i]) {
        (*buffer)[i] = (struct led_t) {
            .brightness = 0
          , .red = 0
          , .green = 0
          , .blue = 0
        };
      }
    }
  }

  // Set frame_number to its next value
  ++frame_number;

  // Read newly fired pulses
  while (current_pulse != pulses_end && current_pulse->time < frame_number) {
    (*buffer)[current_pulse->led_index] = current_pulse->led;
    ++current_pulse;
  }

  // TODO Determination of event display ending

}
