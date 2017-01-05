#ifndef MEMSPACE_H
#define MEMSPACE_H

/** \file
  * \brief Non-uniform memory access.
  * \details The ATmega32U4 has SRAM, flash and EEPROM to store data. In case dynamic information
  *   is mixed with static, perhaps device specific, information, a single interface is provided
  *   to access these different parts of the microcontroller's memory space.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include <stddef.h>

/// Define which memory space the body of a data block is in.
enum memspace_t {
    MEMSPACE_RAM ///< RAM storage.
  , MEMSPACE_PROGMEM ///< Flash (or program memory) storage.
  , MEMSPACE_EEPROM ///< EEPROM storage.
  , MEMSPACE_NONE ///< Placeholder if the refering entity only provides metadata.
};

/// Generic memcpy function that calls the correct copy routine depending on the provided
/// memory space.
void* memcpy_memspace(
    enum memspace_t memspace
  , void* restrict dest
  , const void* restrict src
  , size_t length
);

#endif // MEMSPACE_H
