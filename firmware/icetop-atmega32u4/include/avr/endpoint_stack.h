#ifndef AVR_ENDPOINT_STACK_H
#define AVR_ENDPOINT_STACK_H

#include <stdbool.h>
#include <stdint.h>

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
#define EP_STACK_DEPTH 16

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

#endif //AVR_ENDPOINT_STACK_H
