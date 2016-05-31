#ifndef USB_FIFO_H
#define USB_FIFO_H

/** \file
  * \brief USB endpoint FIFO access.
  * \details A `memcpy`-like interface is provided to read from, and write to, the USB endpoint
  *   FIFOs. After selecting the required endpoint by writing to the `UENUM` register, all function
  *   calls definded in this file
  * \author Sander Vanheule (Universiteit Gent)
  * \see [ATmega32U4 documentation §21-22](http://www.atmel.com/devices/ATMEGA32U4.aspx)
  */

#include <stddef.h>
#include <stdint.h>

/// Current number of bytes in the FIFO.
uint16_t fifo_byte_count();

/// Total capacity of the endpoint FIFO.
uint16_t fifo_size();

/// Write a single byte to the selected FIFO.
void fifo_write_byte(const uint8_t data);

/// Copy a block of memory of \a length bytes, starting at \a data from RAM to the FIFO.
size_t fifo_write(const void* restrict data, const size_t length);

/// Copy a block of memory of \a length bytes, starting at \a data from flash to the FIFO.
size_t fifo_write_P(const void* restrict data, const size_t length);

/// Copy a block of memory of \a length bytes from the FIFO to RAM, starting at \a data.
size_t fifo_read(void* restrict buffer, size_t length);

#endif
