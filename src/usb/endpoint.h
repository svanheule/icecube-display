#ifndef USB_ENDPOINT_H
#define USB_ENDPOINT_H

/** \file
  * \brief Configuration of USB endpoint hardware and memory.
  * \details Endpoint configuration is currently only focussed on control endpoints, so provisions
  *   for other endpoint types are rather minimal.
  *   For the default control endpoint (EP 0) the following config could be used:
  *   ~~~{.c}
  *   static const struct ep_hw_config_t EP_0 = {
  *       // Endpoint number 0
  *       0,
  *       // ... is a control endpoint
  *       EP_TYPE_CONTROL,
  *       // ... with a single 64B bank, the maximum allowed by USB 2.0 for a full-speed device.
  *       EP_BANK_SIZE_64 | EP_BANK_COUNT_1
  *   };
  *   ~~~
  * \author Sander Vanheule (Universiteit Gent)
  * \see [ATmega32U4 documentation §21-22](http://www.atmel.com/devices/ATMEGA32U4.aspx)
  */

#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>


struct ep_hw_config_t {
  /// Endpoint number ranging from 0 to 5.
  uint8_t num;
  /// Endpoint type flags: an ::ep_type_t constant.
  uint8_t config_type;
  /// Endpoint size flags: combination of ::ep_bank_size_t and ::ep_bank_count_t.
  uint8_t config_bank;
};

#define EP_TYPE_CONTROL 0
#define EP_TYPE_ISOCHRONOUS (1<<6)
#define EP_TYPE_BULK (2<<6)
#define EP_TYPE_INTERRUPT (3<<6)
#define EP_DIR_IN 1
#define EP_DIR_OUT 0

enum ep_type_t {
    EP_CONTROL = EP_TYPE_CONTROL
  , EP_ISOCHRONOUS_IN = EP_TYPE_ISOCHRONOUS | EP_DIR_IN
  , EP_ISOCHRONOUS_OUT = EP_TYPE_ISOCHRONOUS | EP_DIR_OUT
  , EP_INTERRUPT_IN = EP_TYPE_INTERRUPT | EP_DIR_IN
  , EP_INTERRUPT_OUT = EP_TYPE_INTERRUPT | EP_DIR_OUT
  , EP_BULK_IN = EP_TYPE_BULK | EP_DIR_IN
  , EP_BULK_OUT = EP_TYPE_BULK | EP_DIR_OUT
};

enum ep_bank_size_t {
    EP_BANK_SIZE_8 = 0
  , EP_BANK_SIZE_16 = (1<<4)
  , EP_BANK_SIZE_32 = (2<<4)
  , EP_BANK_SIZE_64 = (3<<4)
  , EP_BANK_SIZE_128 = (4<<4)
  , EP_BANK_SIZE_256 = (5<<5)
  , EP_BANK_SIZE_512 = (6<<4)
};

enum ep_bank_count_t {
    EP_BANK_COUNT_1 = 0
  , EP_BANK_COUNT_2 = (1<<2)
};

/** \brief Initialise the USB endpoint described by \a config.
  * \details This will allocate the hardware as described by the ATmega32U4 manual and enable
  *   the required interrupts.
  *   For control endpoints, this is `RXSTPI` or 'setup request received' interrupt.
  *   For OUT endpoints (bulk, interrupt, and isochronous) this is `RXOUTI` or 'OUT data received'.
  *   For IN endpoints no interrupts are currently enabled, since none are currently used.
  * \returns `true` if the endpoint configuration was succesful, `false` otherwise.
  */
bool endpoint_configure(const struct ep_hw_config_t* config);

/// Deallocates the endpoint memory.
void endpoint_deconfigure(const uint8_t ep_num);

#endif
