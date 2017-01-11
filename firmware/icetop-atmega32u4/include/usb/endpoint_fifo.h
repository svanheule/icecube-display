#ifndef USB_FIFO_H
#define USB_FIFO_H

/** \file
  * \brief USB endpoint FIFO access.
  * \details A `memcpy`-like interface is provided to read from, and write to, the USB endpoint
  *   FIFOs.
  *
  *   USB communication happens asynchronously to microcontroller operation. Any data
  *   transferred to one of the device's USB endpoints is stored in the memory buffer
  *   associated with this endpoint.
  *   Since this endpoint memory is separated from the rest of the microcontroller's RAM,
  *   this memory has to be read out via a special register acting as a FIFO buffer.
  *   Before reading from or writing to an endpoint's FIFO, the endpoint needs to be
  *   selected. See \ref usb_endpoint_stack for more information.
  *
  *   ~~~{.c}
  *   // Read all bytes currently stored in the FIFO
  *   uint8_t buffer[64];
  *   fifo_read(&buffer[0], fifo_byte_count());
  *
  *   // Write a buffer to the endpoint FIFO
  *   uint8_t buffer[] = {...};
  *   fifo_write(&buffer[0], sizeof(buffer));
  *   ~~~
  * \author Sander Vanheule (Universiteit Gent)
  * \see [ATmega32U4 documentation §21-22](http://www.atmel.com/devices/ATMEGA32U4.aspx)
  */

#include <stddef.h>
#include <stdint.h>

/// \name FIFO information
/// @{

/// Current number of bytes in the FIFO.
uint16_t fifo_byte_count();

/// Total capacity of the endpoint FIFO.
uint16_t fifo_size();

/// @}

/// \name FIFO data access
/// @{

/** \brief Copy a block of data of from RAM to the FIFO.
  * \param data Pointer to the data block.
  * \param length Number of bytes to copy.
  * \returns The number of bytes actually written to the FIFO. This may be less than \a length.
  */
size_t fifo_write(const void* restrict data, const size_t length);

/** \brief Copy a block of data of from flash to the FIFO.
  * \param data Pointer to the data block.
  * \param length Number of bytes to copy.
  * \returns The number of bytes actually written to the FIFO. This may be less than \a length.
  */
size_t fifo_write_P(const void* restrict data, const size_t length);

/** \brief Copy a block of data of the FIFO to RAM.
  * \param buffer Pointer to destination memory.
  * \param length Number of bytes to copy.
  * \returns The number of bytes actually read from the FIFO. This may be less than \a length.
  */
size_t fifo_read(void* restrict buffer, size_t length);

/// @}

#endif
