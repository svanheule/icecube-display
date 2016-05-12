#include "render/boot_splash.h"
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

#define LED_OFF (struct led_t) {0, 0, 0, 0}
#define LED_UGENT_BLUE (struct led_t) {0x08, 10, 30, 96}
static const uint8_t UGENT_LOGO[LED_COUNT] PROGMEM = {
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

void init_splash() {
  ugent_frame = create_frame();
  if (ugent_frame) {
    struct led_t* led_ptr = ugent_frame->buffer;
    for (uint8_t led = 0; led < LED_COUNT; ++led) {
      if (pgm_read_byte(&UGENT_LOGO[led]) == 0) {
        *led_ptr = (struct led_t) {0, 0, 0, 0};
      }
      else {
        *led_ptr = (struct led_t) {0x08, 10, 30, 96};
      }
      ++led_ptr;
    }
  }
}

void stop_splash() {
  free(ugent_frame);
  ugent_frame = 0;
}

struct frame_buffer_t* render_splash() {
  return ugent_frame;
}
