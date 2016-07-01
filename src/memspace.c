#include "memspace.h"
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <string.h>

void* memcpy_memspace(enum memspace_t memspace, void* dest, const void* src, size_t length) {
  switch (memspace) {
    case MEMSPACE_RAM:
      return memcpy(dest, src, length);
      break;
    case MEMSPACE_PROGMEM:
      return memcpy_P(dest, src, length);
      break;
    case MEMSPACE_EEPROM:
      eeprom_read_block(dest, src, length);
      return dest;
      break;
    default:
      return dest;
      break;
  }
}
