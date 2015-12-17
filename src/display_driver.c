#include <avr/io.h>
#include "display_driver.h"

// TODO Define a look-up table for tank-number -> led-position conversion
// {6..1}+{7..13}+{21..14}+{22..30}+{40..31}+{41..50}+{59..51}+{60..67}+{74..68}+{75..78}

void init_driver() {
  // Enable SPI, set as master
  // Transmit MSB first, idle low, transmit on first (rising) edge, SCK=fOSC/4
  SPCR = (1 << SPE)|(1 << MSTR) | (0 << DORD)|(0 << CPOL)|(0 << CPHA);
//  SPSR = (1 << SPI2X); not necessary, now running at 16/4 MHz = 4 MHz (~times 0.5 idling overhead)
}

static inline void wait_write_finish () {
  // Check transmission finished bit
  while ( !(SPSR & (1<<SPIF)) ) {}
}

static inline void write_byte_no_block(const unsigned char byte) {
  SPDR = byte;
}

static inline void write_byte(const unsigned char byte) {
  // Write next byte
  write_byte_no_block(byte);
  // Wait for write to finish
  wait_write_finish();
}

/**
 * display_frame should be able to handle interruptions and be performed in the main loop.
 * The CTC compare interrupt should just wake the main loop, and then return.
 * In this way, the interrupt duration is kept to a minimum, which ensures that the USART Rx ISR
 * can be called in due time to write a newly received byte to the next frame.
 */
void display_frame(const frame_t* buffer) {
  /* # APA102C
   * Transmit bytes with MSB first
   * Data frame
   * * frame start: 4 bytes 0x00
   * * led data: (111X:XXXX BBBB:BBBB GGGG:GGGG RRRR:RRRR)
   *   * '111' header + 5 bits global brightness
   *   * blue, green, red
   * * frame end: 4 bytes 0xFF
   */
  const unsigned char FRAME_HEADER = 0x00;
  const unsigned char FRAME_FOOTER = 0xFF;
  const unsigned char LED_HEADER = 0xE0;

  if (buffer) {
    unsigned char i;
    // Start of frame
    for (i = 0; i < 4; ++i) {
      write_byte(FRAME_HEADER);
    }

    const struct led_t* leds = *buffer;

    // LED data
    for (i = 0; i < LED_COUNT; ++i) {
      write_byte(LED_HEADER | 0x08);
      write_byte(leds[i].blue);
      write_byte(leds[i].green);
      write_byte(leds[i].red);
    }

    // End of frame
    for (i = 0; i < LED_COUNT; i+=16) {
      write_byte(FRAME_FOOTER);
    }

  }
}

