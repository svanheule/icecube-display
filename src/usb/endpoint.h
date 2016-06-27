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
  *
  * \defgroup usb_endpoint USB endpoint operations
  * \details All communications on the USB bus happen between the host and a device endpoint.
  *   A number of functions is provided to facilitate endpoint manipulation on the microcontroller.
  *   Since the ATmega32U4 microcontroller only allows access to one endpoint at a time, an
  *   \ref usb_endpoint_stack "endpoint selection system" is provided.
  */

#include <stdbool.h>
#include <stdint.h>


/** \defgroup usb_endpoint_config Endpoint configuration
  * \ingroup usb_endpoint
  * \brief Configuration and memory allocation of USB endpoints.
  * \details To configure a USB endpoint, the USB controller needs to know what type it is and
  *   how much memory it needs.
  *   The endpoint configuration definitions and types are currently specific to the AVR hardware,
  *   but this may change in the future if this firmware if ever ported to work on Teensy-like
  *   microcontrollers with an ARM CPU.
  * @{
  */

struct ep_hw_config_t {
  /// Endpoint number ranging from 0 to 5.
  uint8_t num;
  /// Endpoint type flags: an ::ep_type_t constant.
  uint8_t config_type;
  /// Endpoint size flags: combination of ::ep_bank_size_t and ::ep_bank_count_t.
  uint8_t config_bank;
};

/// \name Endpoint types
/// @{
#define EP_TYPE_CONTROL 0
#define EP_TYPE_ISOCHRONOUS 1
#define EP_TYPE_BULK 2
#define EP_TYPE_INTERRUPT 3
/// @}

/// \name Endpoint directions
/// @{
#define EP_DIR_IN 1
#define EP_DIR_OUT 0
/// @}

/// List of endpoint types
enum ep_type_t {
    EP_CONTROL = EP_TYPE_CONTROL
  , EP_ISOCHRONOUS_IN = (EP_TYPE_ISOCHRONOUS << 6) | EP_DIR_IN
  , EP_ISOCHRONOUS_OUT = (EP_TYPE_ISOCHRONOUS << 6) | EP_DIR_OUT
  , EP_INTERRUPT_IN = (EP_TYPE_INTERRUPT << 6) | EP_DIR_IN
  , EP_INTERRUPT_OUT = (EP_TYPE_INTERRUPT << 6) | EP_DIR_OUT
  , EP_BULK_IN = (EP_TYPE_BULK << 6) | EP_DIR_IN
  , EP_BULK_OUT = (EP_TYPE_BULK << 6)| EP_DIR_OUT
};

/// Enumeration of valid endpoint FIFO buffer sizes.
enum ep_bank_size_t {
    EP_BANK_SIZE_8 = 0
  , EP_BANK_SIZE_16 = (1<<4)
  , EP_BANK_SIZE_32 = (2<<4)
  , EP_BANK_SIZE_64 = (3<<4)
  , EP_BANK_SIZE_128 = (4<<4)
  , EP_BANK_SIZE_256 = (5<<5)
  , EP_BANK_SIZE_512 = (6<<4)
};

/** \brief Endpoint FIFO bank counts
  * \details An endpoint can use either one or two buffers. Using only one bank saves memory,
  * but also requires the buffer to be emptied before the endpoint can resume operation.
  * With two banks, the buffers are used in a ping-pong fashion, allowing for simultaneous use
  * of the buffers by the endpoint hardware and the firmware. While one buffer is used by the
  * hardware, the firmware can read/write to the other. This may allow for higher throughputs as
  * the endpoint doesn't have to wait for the firmware to finish to transmit or receive more data.
  */
enum ep_bank_count_t {
    EP_BANK_COUNT_1 = 0 ///< Use a single endpoint buffer. Has to be emptied before re-use.
  , EP_BANK_COUNT_2 = (1<<2) ///< Use ping-pong buffers. Allows for higher data throughput.
};

/** \brief Initialise the USB endpoint described by \a config.
  * \details This will allocate the hardware as described by the ATmega32U4 manual and enable
  *   the required interrupts. Note that endpoints should be allocated starting from endpoint 0
  *   up to the last endpoint. Failing to do so will likely result in endpoint memory corruption.
  *
  *   For control endpoints, this is `RXSTPI` or 'setup request received' interrupt.
  *   For OUT endpoints (bulk, interrupt, and isochronous) this is `RXOUTI` or 'OUT data received'.
  *   For IN endpoints no interrupts are currently enabled, since none are currently used.
  * \returns `true` if the endpoint configuration was succesful, `false` otherwise.
  */
bool endpoint_configure(const struct ep_hw_config_t* config);

/// Deallocates the endpoint memory.
void endpoint_deconfigure(const uint8_t ep_num);

/// @}


// Endpoint selection stack

/** \defgroup usb_endpoint_stack Endpoint selection
  * \ingroup usb_endpoint
  * \brief Selection of the currently active endpoint.
  * \details On the ATmega32U4 only one USB endpoint can be accessed at a time.
  *   This is done by writing the endpoint number to the `UENUM` special function register.
  *   When using interrupts, it may happen that an operation on one endpoint is interrupted to
  *   perform an operation on another endpoint. In this case, the original endpoint number
  *   should be stored before selecting the new endpoint, and restored after the operation on the
  *   new endpoint is finished.
  *
  *   An endpoint stack is provided to simplify this bookkeeping. Endpoint selection is done via
  *   a call to endpoint_push() and automatically stores the previously selected endpoint.
  *   When the endpoint can be released again, endpoint_pop() should be called to ensure the
  *   previous endpoint is restored.
  *   ::EP_STACK_DEPTH determines how many endpoint operations can be performed simultaneously
  *   using the endpoint stack.
  *
  *   ~~~{.c}
  *   // Select endpoint 0 by pushing it to the top of the stack.
  *   endpoint_push(0);
  *
  *   // Read 64 bytes from endpoint 0
  *   uint8_t buffer[64];
  *   fifo_read(&(buffer[0]), 64);
  *
  *   // Restore previous endpoint by popping the current endpoint from the top of the stack.
  *   endpoint_pop();
  *   ~~~
  * @{
  */

/// Depth of the endpoint number stack.
#define EP_STACK_DEPTH  5

/** \brief Select a USB endpoint.
  * \details Selects endpoint number \a ep_num and stores the previously selected endpoint.
  * \param ep_num Number of endpoint to be selected.
  * \returns `true` if there was room to store the previous endpoint, `false` otherwise.
  */
bool endpoint_push(const uint8_t ep_num);

/** \brief Restore the previously selected endpoint.
  * \returns `true` if there was an endpoint to be restored, `false` otherwise.
  */
bool endpoint_pop();

/// @}

#endif
