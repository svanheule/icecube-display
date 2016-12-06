#include "usb/led.h"

#include "kinetis/io.h"
#include "kinetis/pit.h"

#include <stdbool.h>
#include <stdint.h>


// USB_ACT LED control
static enum led_mode_t led_mode;
static bool led_tripped;
static bool interval_end;

static inline void set_led_on() {
  GPIOC_PSOR = _BV(5);
}

static inline void set_led_off() {
  GPIOC_PCOR = _BV(5);
}

#define INTERVALS_PER_SECOND 10

static void disable_timer() {
  NVIC_DISABLE_IRQ(IRQ_PIT_CH1);
  pit_channels[1].TCTRL = 0;
}

static void enable_timer(uint8_t interval_count) {
  // Disable channel and clear pending interrupts
  pit_channels[1].TCTRL = 0;
  pit_channels[1].TFLG = 1;
  NVIC_ENABLE_IRQ(IRQ_PIT_CH1);

  interval_end = false;
  pit_channels[1].LDVAL = (F_BUS/INTERVALS_PER_SECOND/2)*interval_count - 1;
  pit_channels[1].TCTRL = _BV(1); // enable timer interrupts
  pit_channels[1].TCTRL = _BV(1)|_BV(0); // ... and enable timer
}

void init_led() {
  // Ensure PORTC_PCR5 is configured for GPIO output
  ATOMIC_REGISTER_BIT_SET(SIM_SCGC5, 11); // Enable port C clock
  ATOMIC_REGISTER_BIT_SET(GPIOC_PDDR, 5); // Configure C5 as output
  PORTC_PCR5 = (1<<8); // GPIO mode

  // Enable PIT module
  enable_pit_module();

  set_led_state(LED_OFF);
}

void set_led_state(const enum led_mode_t mode) {
  led_mode = mode;
  switch (mode) {
    case LED_OFF:
      disable_timer();
      set_led_off();
      break;
    case LED_BLINK_SLOW:
      set_led_on();
      enable_timer(INTERVALS_PER_SECOND);
      break;
    case LED_TRIP_FAST:
      set_led_on();
      led_tripped = false;
      enable_timer(1);
      break;
  }
}

void trip_led() {
  led_tripped = true;
}

void pit1_isr() {
  pit_channels[1].TFLG = 1;

  if (interval_end) {
    // Reset LED at end of cycle
    set_led_on();
  }
  else {
    // Toggle LED if it was tripped
    if (led_mode == LED_BLINK_SLOW || led_tripped) {
      set_led_off();
      led_tripped = false;
    }
  }

  interval_end = !interval_end;
}
