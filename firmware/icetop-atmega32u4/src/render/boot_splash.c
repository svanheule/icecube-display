#include "render/boot_splash.h"
#include "frame_buffer.h"
#include <avr/pgmspace.h>
#include <stdlib.h>

static void init_splash();
static void stop_splash();
static struct frame_buffer_t* render_splash();

static const struct renderer_t SPLASH_RENDERER = {
    init_splash
  , stop_splash
  , render_splash
};

const struct renderer_t* get_boot_splash_renderer() {
  return &SPLASH_RENDERER;
}

static const uint8_t UGENT_LOGO[LED_COUNT_IT78] PROGMEM = {
        0,0,0,0,0,0
     , 0,0,0,0,0,0,0
    , 1,1,1,1,1,1,1,1
   , 0,1,0,1,0,1,0,1,0
  , 0,0,0,0,0,0,0,0,0,0
   , 0,1,0,1,0,1,0,1,0,0
    , 1,1,1,1,1,1,1,1,0
     , 1,0,0,0,0,0,1,0
      , 0,1,0,0,1,0,0
       , 0,0,1,0
};


static struct frame_buffer_t* ugent_frame;

void init_splash()  {
  if (!ugent_frame) {
    ugent_frame = create_frame();
  }
  if (ugent_frame) {
    const uint8_t led_count = get_led_count();
    struct led_t* led_ptr = ugent_frame->buffer;
    uint8_t led = 0;
    while (led < LED_COUNT_IT78) {
      if (pgm_read_byte(&UGENT_LOGO[led]) == 0) {
        *led_ptr = (struct led_t) {0, 0, 0, 0};
      }
      else {
        *led_ptr = (struct led_t) {0x08, 10, 30, 96};
      }
      ++led_ptr;
      ++led;
    }
    // Clear remaining LEDs
    while (led < led_count) {
      *led_ptr = (struct led_t) {0, 0, 0, 0};
      ++led_ptr;
      ++led;
    }
  }
}

void stop_splash() {
  if (ugent_frame) {
    destroy_frame(ugent_frame);
    ugent_frame = 0;
  }
}

struct frame_buffer_t* render_splash() {
  return ugent_frame;
}
