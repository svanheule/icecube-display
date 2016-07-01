#ifndef MEMSPACE_H
#define MEMSPACE_H

#include <stdlib.h>

/// Define which memory space the body of a data block is in.
enum memspace_t {
    MEMSPACE_RAM
  , MEMSPACE_PROGMEM
  , MEMSPACE_EEPROM
  , MEMSPACE_NONE
};

void* memcpy_memspace(
    enum memspace_t memspace
  , void* restrict dest
  , const void* restrict src
  , size_t length
);

#endif // MEMSPACE_H
