#ifndef REMOTE_H
#define REMOTE_H

/**
  * \file
  * \brief Remote connection interface
  * \details When using the remote connection on the hardware, the correct ports and internal
  *   state need to be initialised first with init_remote().
  *   is_remote_connected() can then be used to determine whether there is an active connection.
  *   The firmware can use this to switch off any local renderer, and wait for incoming data on
  *   the connection.
  *   Different implementations are provided depending on whether the remote connection is via
  *   USB or UART.
  * \author Sander Vanheule (Universiteit Gent)
  * \see remote_usb.c
  * \see remote_serial.c
  */

#include <stdbool.h>

/** \brief Initialise the remote connection.
 * \details If this is not called, is_remote_connected will *always* return false
 */
void init_remote();

/** \brief Check remote connection.
 * \returns `true` if there is an active connection between the display and another device,
 *   `false` otherwise.
 */
bool is_remote_connected();

#endif
