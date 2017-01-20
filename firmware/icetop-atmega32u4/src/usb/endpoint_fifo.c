#include "usb/endpoint_fifo.h"
#include <avr/io.h>
#include <avr/pgmspace.h>

#define BYTE_COUNT() ((((uint16_t) UEBCHX) << 8) | (UEBCLX))

uint16_t fifo_byte_count() {
  return BYTE_COUNT();
}

uint16_t fifo_size() {
  uint8_t size_power = (UECFG1X & (0x7 << EPSIZE0)) >> EPSIZE0;
  return 1 << (3 + size_power);
}

size_t fifo_write(const void* restrict data, const size_t length) {
  const uint8_t* tmp = (const uint8_t*) data;
  size_t remaining = length;
  while (remaining--) {
    UEDATX = *(tmp++);
  }
  return length;
}

size_t fifo_write_P(const void* restrict data, const size_t length) {
  const uint8_t* tmp = (const uint8_t*) data;
  size_t remaining = length;
  while (remaining--) { 
    UEDATX = pgm_read_byte(tmp++);
  }
  return length;
}

size_t fifo_read(void* restrict buffer, size_t length) {
  size_t read = 0;
  uint8_t* write_ptr = (uint8_t*) buffer;

  while (BYTE_COUNT() && read < length) {
    *(write_ptr++) = UEDATX;
    read++;
  }

  return read;
}
