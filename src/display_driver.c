#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include "display_driver.h"
#include "display_properties.h"

static const uint8_t LED_MAP_IT78[LED_COUNT_IT78] PROGMEM = {
              5,  4,  3,  2,  1,  0
          ,  6,  7,  8,  9, 10, 11, 12
        , 20, 19, 18, 17, 16, 15, 14, 13
      , 21, 22, 23, 24, 25, 26, 27, 28, 29
    , 39, 38, 37, 36, 35, 34, 33, 32, 31, 30
  , 40, 41, 42, 43, 44, 45, 46, 47, 48, 49
    , 58, 57, 56, 55, 54, 53, 52, 51, 50
      , 59, 60, 61, 62, 63, 64, 65, 66
        , 73, 72, 71, 70, 69, 68, 67
          , 74, 75, 76, 77
};
static const uint8_t LED_MAP_IT81[LED_COUNT_IT81] PROGMEM = {
              5,  4,  3,  2,  1,  0
          ,  6,  7,  8,  9, 10, 11, 12
        , 20, 19, 18, 17, 16, 15, 14, 13
      , 21, 22, 23, 24, 25, 26, 27, 28, 29
    , 39, 38, 37, 80, 35, 79, 36, 78, 34, 33, 32, 31, 30
  , 40, 41, 42, 43, 44, 45, 46, 47, 48, 49
    , 58, 57, 56, 55, 54, 53, 52, 51, 50
      , 59, 60, 61, 62, 63, 64, 65, 66
        , 73, 72, 71, 70, 69, 68, 67
          , 74, 75, 76, 77
};

static uint8_t led_count;
static const uint8_t* led_mapping_P;

static uint8_t offset_c1;
static uint8_t offset_c2;
static uint8_t offset_c3;

#define OFFSET_RED offsetof(struct led_t, red)
#define OFFSET_GREEN offsetof(struct led_t, green)
#define OFFSET_BLUE offsetof(struct led_t, blue)

void init_display_driver() {
#if defined(CONTROLLER_ARDUINO)
  /* On the Arduino's ATmega328p the hardware SPI port is used, located on port B.
   * DDRB must be set before SPCR, so the internal pull-up doens't cause SPI to go into slave mode
   * Configure port B as SPI master:
   * * B5: SCK (out)
   * * B4: MISO (in, unused)
   * * B3: MOSI (out)
   * * B2: /SS (out, unused)
   */
  DDRB = _BV(DDB5)|_BV(DDB3)|_BV(DDB2);
  // Enable SPI, set as master
  // Transmit MSB first, idle low, transmit on first (rising) edge, SCK=fOSC/4 = 4MHz
  SPCR = _BV(SPE)|_BV(MSTR);
  SPSR = _BV(SPI2X);
#elif defined(CONTROLLER_UGENT)
  /* ATmega32U4 design uses the USART port in master SPI mode to drive the LED string.
   * The USART pins are located on port D:
   * * D2: RXD1 (MISO, in, unused)
   * * D3: TXD1 (MOSI, out)
   * * D5: XCK1 (SCK, out)
   */
#define SPI_BAUD_RATE 4000000UL
  DDRD |= _BV(DDD5) | _BV(DDD3);
  // USART as master SPI
  UCSR1B = _BV(TXEN1);
  UCSR1C = _BV(UMSEL11) | _BV(UMSEL10);
  // Set baud rate to 4M, using 16MHz system clock
  const uint16_t baud_rate_register = ((F_CPU + SPI_BAUD_RATE)/(2*SPI_BAUD_RATE) - 1);
  UBRR1H = (uint8_t) ((baud_rate_register >> 8) & 0x0F);
  UBRR1L = (uint8_t) baud_rate_register;
#undef SPI_BAUD_RATE
#endif

  // Load LED layout settings
  led_count = get_led_count();
  switch (led_count) {
    case LED_COUNT_IT78:
      led_mapping_P = LED_MAP_IT78;
      break;
    case LED_COUNT_IT81:
      led_mapping_P = LED_MAP_IT81;
      break;
    default:
      led_count = 0;
      led_mapping_P = 0;
      break;
  }

  enum display_led_color_order_t color_order = get_color_order();

  switch (color_order) {
    case LED_ORDER_BGR:
      offset_c1 = OFFSET_BLUE;
      offset_c2 = OFFSET_GREEN;
      offset_c3 = OFFSET_RED;
      break;
    case LED_ORDER_BRG:
      offset_c1 = OFFSET_BLUE;
      offset_c2 = OFFSET_RED;
      offset_c3 = OFFSET_GREEN;
      break;
    case LED_ORDER_GBR:
      offset_c1 = OFFSET_GREEN;
      offset_c2 = OFFSET_BLUE;
      offset_c3 = OFFSET_RED;
      break;
    case LED_ORDER_GRB:
      offset_c1 = OFFSET_GREEN;
      offset_c2 = OFFSET_RED;
      offset_c3 = OFFSET_BLUE;
      break;
    case LED_ORDER_RBG:
      offset_c1 = OFFSET_RED;
      offset_c2 = OFFSET_BLUE;
      offset_c3 = OFFSET_GREEN;
      break;
    case LED_ORDER_RGB:
      offset_c1 = OFFSET_RED;
      offset_c2 = OFFSET_GREEN;
      offset_c3 = OFFSET_BLUE;
      break;
  }

}

static inline void wait_write_finish () {
  // Check transmission finished bit
#if defined(CONTROLLER_ARDUINO)
  while ( !(SPSR & _BV(SPIF)) ) {}
#elif defined(CONTROLLER_UGENT)
  while ( !(UCSR1A & _BV(UDRE1)) ) {}
#endif
}

static inline void write_byte_no_block(const uint8_t byte) {
#if defined(CONTROLLER_ARDUINO)
  SPDR = byte;
#elif defined(CONTROLLER_UGENT)
  UDR1 = byte;
#endif
}

static inline void write_byte(const uint8_t byte) {
  // Write next byte
  write_byte_no_block(byte);
  // Wait for write to finish
  wait_write_finish();
}


/* # APA102C
 * Transmit bytes with MSB first
 * Data frame
 * * frame start: 4 bytes 0x00
 * * led data: (111X:XXXX BBBB:BBBB GGGG:GGGG RRRR:RRRR)
 *   * '111' header + 5 bits global brightness
 *   * blue, green, red
 * * frame end: ceil(n/2) '1' bits, or ceil(n/2/8) 0xFF bytes
 */

static inline void write_frame_header() {
  const uint8_t FRAME_HEADER = 0x00;
  for (uint8_t i = 0; i < 4; ++i) {
    write_byte(FRAME_HEADER);
  }
}

static inline void write_frame_footer() {
  const uint8_t FRAME_FOOTER = 0xFF;
  for (uint8_t i = 0; i < led_count; i+=16) {
    write_byte(FRAME_FOOTER);
  }
}

const uint8_t LED_HEADER = 0xE0;

void display_frame(struct frame_buffer_t* frame) {
  frame->flags |= FRAME_DRAW_IN_PROGRESS;

  const uint8_t* leds = (const uint8_t*) frame->buffer;

  // Start of frame
  write_frame_header();

  // LED data
  uint16_t index;
  for (uint8_t station = 0; station < led_count; ++station) {
    index = 4*pgm_read_byte(&led_mapping_P[station]);
    write_byte(LED_HEADER | leds[index]);
    write_byte(leds[index+offset_c1]);
    write_byte(leds[index+offset_c2]);
    write_byte(leds[index+offset_c3]);
  }

  // End of frame
  write_frame_footer();

  frame->flags &= ~FRAME_DRAW_IN_PROGRESS;
}


void display_blank() {
  // Start of frame
  write_frame_header();

  // LED data
  for (uint8_t i = 0; i < led_count; ++i) {
    write_byte(LED_HEADER);
    write_byte(0);
    write_byte(0);
    write_byte(0);
  }

  // End of frame
  write_frame_footer();
}
