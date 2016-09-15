#ifndef KINETIS_IO_H
#define KINETIS_IO_H

#include <stddef.h>
#include <kinetis.h>

#define _BV(n) (1<<(n))

#define BITBAND_REGISTER_ADDRESS(reg_addr, bit) \
  ( (volatile uint32_t*) (0x42000000 + ((uint32_t)(reg_addr-0x40000000))*32 + bit*4) )
#define ATOMIC_REGISTER_BIT_SET(reg, bit) *BITBAND_REGISTER_ADDRESS(&reg, bit) = 1
#define ATOMIC_REGISTER_BIT_CLEAR(reg, bit) *BITBAND_REGISTER_ADDRESS(&reg, bit) = 0

#define BITBAND_SRAM_ADDRESS(reg_addr, bit) \
  ( (volatile uint32_t*) (0x22000000 + ((uint32_t)(reg_addr-0x20000000))*32 + bit*4) )
#define ATOMIC_SRAM_BIT_SET(reg, bit) *BITBAND_SRAM_ADDRESS(&reg, bit) = 1
#define ATOMIC_SRAM_BIT_CLEAR(reg, bit) *BITBAND_SRAM_ADDRESS(&reg, bit) = 0


#endif
