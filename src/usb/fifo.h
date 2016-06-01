#ifndef USB_FIFO_H
#define USB_FIFO_H

/** \file
  * \brief USB endpoint FIFO access.
  * \details A `memcpy`-like interface is provided to read from, and write to, the USB endpoint
  *   FIFOs.
  * \author Sander Vanheule (Universiteit Gent)
  * \see [ATmega32U4 documentation §21-22](http://www.atmel.com/devices/ATMEGA32U4.aspx)
  */

#include <stddef.h>
#include <stdint.h>

/** \defgroup usb_endpoint_fifo Endpoint FIFO operations
  * \ingroup usb_endpoint
  * \details USB communication happens asynchronously to microcontroller operation. Any data
  *   transferred to one of the device's endpoints is therefore stored in the memory buffer
  *   associated with tis endpoint.
  * @{
  */

/// Current number of bytes in the FIFO.
uint16_t fifo_byte_count();

/// Total capacity of the endpoint FIFO.
uint16_t fifo_size();

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
