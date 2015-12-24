#include <avr/io.h>
#include <stdint.h>
#include "display_driver.h"

// TODO Define a look-up table for tank-number -> led-position conversion
// {6..1}+{7..13}+{21..14}+{22..30}+{40..31}+{41..50}+{59..51}+{60..67}+{74..68}+{75..78}

void init_display_driver() {
  // DDRB must be set before SPCR, so the internal pull-up doens't cause SPI to go into slave mode
  /* Configure port B as SPI master:
   * * B5: SCK (out)
   * * B4: MISO (in, unused)
   * * B3: MOSI (out)
   * * B2: /SS (out, unused)
   */
  DDRB &= (1<<DDB7) | (1<<DDB6); // Clear all but the clock pins
  DDRB |= (1<<DDB5) | (1<<DDB3) | (1<<DDB2);
  // Enable SPI, set as master
  // Transmit MSB first, idle low, transmit on first (rising) edge, SCK=fOSC/4
  SPCR = (1 << SPE)|(1 << MSTR) | (0 << DORD)|(0 << CPOL)|(0 << CPHA);
//  SPSR = (1 << SPI2X); not necessary, now running at 16/4 MHz = 4 MHz (~times 0.5 idling overhead)
}

static inline void wait_write_finish () {
  // Check transmission finished bit
  while ( !(SPSR & (1<<SPIF)) ) {}
}

static inline void write_byte_no_block(const uint8_t byte) {
  SPDR = byte;
}

static inline void write_byte(const uint8_t byte) {
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
   * * frame end: ceil(n/2) '1' bits, or ceil(n/2/8) 0xFF bytes
   */
  const uint8_t FRAME_HEADER = 0x00;
  const uint8_t FRAME_FOOTER = 0xFF;
  const uint8_t LED_HEADER = 0xE0;

  if (buffer) {
    uint8_t i;
    // Start of frame
    for (i = 0; i < 4; ++i) {
      write_byte(FRAME_HEADER);
    }

    const struct led_t* leds = *buffer;

    // LED data
    for (i = 0; i < LED_COUNT; ++i) {
      write_byte(LED_HEADER | leds[i].brightness);
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

