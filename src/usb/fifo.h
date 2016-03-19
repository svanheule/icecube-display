#ifndef USB_FIFO_H
#define USB_FIFO_H

#include <stddef.h>
#include <stdint.h>

uint16_t fifo_byte_count();
uint16_t fifo_size();

void fifo_write_byte(const uint8_t data);
size_t fifo_write(const void* restrict data, const size_t length);
size_t fifo_write_P(const void* restrict data, const size_t length);

size_t fifo_read(void* restrict buffer, size_t length);

#endif
