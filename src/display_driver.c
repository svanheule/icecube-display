#include <avr/io.h>
#include "display_driver.h"
#include "frame_buffer.h"


void init_driver() {
  // Enable interrupt, set as master, transmit MSB first
  SPCR = (1 << SPE) | (1 << MSTR) | (0 << DORD);
//  SPSR = (1 << SPI2X); not necessary, now running at 16/4 MHz = 4 MHz (~times 0.5 idling overhead)
}

static inline void wait_write_finish () {
  // Check transmission finished bit
  while ( !(SPSR & (1<<SPIF)) ) {}
}

static inline void write_display_byte(const unsigned char byte) {
  // ...then write next byte
  SPDR = byte;
  // Wait for write to finish
  wait_write_finish();
}

/**
 * display_frame should be able to handle interruptions and be performed in the main loop.
 * The CTC compare interrupt should just wake the main loop, and then return.
 * In this way, the interrupt duration is kept to a minimum, which ensures that the USART Rx ISR
 * can be called in due time to write a newly received byte to the next frame.
 */
void display_frame(const unsigned char* buffer) {
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
      write_display_byte(FRAME_HEADER);
    }

    // LED data
    for (i = 0; i < LED_COUNT; ++i) {
      write_display_byte(LED_HEADER | 0x10);
      write_display_byte(buffer[i, BUFFER_BLUE]);
      write_display_byte(buffer[i, BUFFER_GREEN]);
      write_display_byte(buffer[i, BUFFER_RED]);
    }

    // End of frame
    for (i = 0; i < 4; ++i) {
      write_display_byte(FRAME_FOOTER);
    }

  }
}

