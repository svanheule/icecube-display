#ifndef USB_CONFIGURATION_H
#define USB_CONFIGURATION_H

#include <avr/pgmspace.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool valid_configuration_index(int8_t index);
bool set_configuration_index(int8_t index);
int8_t get_configuration_index();

#endif
