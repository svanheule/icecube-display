#include <avr/io.h>
#include <stdint.h>
#include "display_driver.h"

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

