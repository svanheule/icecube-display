#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include "display_types.h"
#include "display_driver.h"
#include "display_properties.h"

// See pocket_icetop.dxf
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

// See pocket_icetop_large.dxf
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

/* Pre-calculated differential offsets between LED color components.
 * Since the LED chips might need the RGB data in a different order than the one that is stored
 * in RAM, calculate and store the pointer differences in the required order.
 * If an led_t object L is stored at address A, then L.red is stored at A+1, etc.
 * The jump_c[1-3] variables store the differences between the transmitted RGB components.
 * If the tranmission order is blue, green, red for example, jump_c[1-3] will be [3,-1,-1].
 * This corresponds to reading the data at [A, A+3, A+2, A+1]
 */
static int8_t jump_c1;
static int8_t jump_c2;
static int8_t jump_c3;

#define OFFSET_RED ((int8_t) offsetof(struct led_t, red))
#define OFFSET_GREEN ((int8_t) offsetof(struct led_t, green))
#define OFFSET_BLUE ((int8_t) offsetof(struct led_t, blue))


/* Since the display uses a number of APA102 LEDs connected in series, a hardware SPI
 * master port can be used to drive the LED chain. On an ATmega microcontroller, this can be
 * either a hardware SPI port, or a USART port configured as SPI master. The latter case allows
 * for lower transmission times as it has a hardware buffer that can be kept filled during
 * transmission. When using a SPI port, the buffer is only the size of the currently transmitted
 * byte, so one has to wait (and check) until the byte is transmitted to load the next byte,
 * introducing a small delay between bytes.
 */
void init_display_driver() {
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

  switch (get_color_order()) {
    case LED_ORDER_BGR:
      jump_c1 = OFFSET_BLUE;
      jump_c2 = OFFSET_GREEN-OFFSET_BLUE;
      jump_c3 = OFFSET_RED-OFFSET_GREEN;
      break;
    case LED_ORDER_BRG:
      jump_c1 = OFFSET_BLUE;
      jump_c2 = OFFSET_RED-OFFSET_GREEN;
      jump_c3 = OFFSET_GREEN-OFFSET_RED;
      break;
    case LED_ORDER_GBR:
      jump_c1 = OFFSET_GREEN;
      jump_c2 = OFFSET_BLUE-OFFSET_GREEN;
      jump_c3 = OFFSET_RED-OFFSET_BLUE;
      break;
    case LED_ORDER_GRB:
      jump_c1 = OFFSET_GREEN;
      jump_c2 = OFFSET_RED-OFFSET_GREEN;
      jump_c3 = OFFSET_BLUE-OFFSET_RED;
      break;
    case LED_ORDER_RBG:
      jump_c1 = OFFSET_RED;
      jump_c2 = OFFSET_BLUE-OFFSET_RED;
      jump_c3 = OFFSET_GREEN-OFFSET_BLUE;
      break;
    case LED_ORDER_RGB:
      jump_c1 = OFFSET_RED;
      jump_c2 = OFFSET_GREEN-OFFSET_RED;
      jump_c3 = OFFSET_BLUE-OFFSET_GREEN;
      break;
  }

}

static inline void wait_write_finish() __attribute__((always_inline));
static inline void wait_write_finish() {
  // Check transmission finished bit
  while ( !(UCSR1A & _BV(UDRE1)) ) {}
}

static inline void write_byte_no_block(const uint8_t byte) __attribute__((always_inline));
static inline void write_byte_no_block(const uint8_t byte) {
  UDR1 = byte;
}

static inline void write_byte(const uint8_t byte) __attribute__((always_inline));
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

static inline void write_frame_header() __attribute__((always_inline));
static inline void write_frame_header() {
  const uint8_t FRAME_HEADER = 0x00;
  for (uint8_t i = 0; i < 4; ++i) {
    write_byte(FRAME_HEADER);
  }
}

static inline void write_frame_footer() __attribute__((always_inline));
static inline void write_frame_footer() {
  const uint8_t FRAME_FOOTER = 0xFF;
  for (uint8_t i = 0; i < led_count; i+=16) {
    write_byte(FRAME_FOOTER);
  }
}

const uint8_t LED_HEADER = 0xE0;

void display_frame(struct frame_buffer_t* frame) {
  frame->flags |= FRAME_DRAW_IN_PROGRESS;

  // Transmit LED data
  const uint8_t* leds = frame->buffer;
  const uint8_t* led_P = led_mapping_P;
  const uint8_t* led_end_P = led_mapping_P + led_count;

  // Start of frame
  write_frame_header();

  while (led_P != led_end_P) {
    // Calculate initial buffer position
    const uint8_t* led_data = leds + sizeof(struct led_t)*pgm_read_byte(led_P++);
    write_byte(LED_HEADER | *led_data);
    // Jump to first, second, and third transmitted component
    led_data += jump_c1;
    write_byte(*led_data);
    led_data += jump_c2;
    write_byte(*led_data);
    led_data += jump_c3;
    write_byte(*led_data);
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
