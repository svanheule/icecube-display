#ifndef USB_REMOTE_RENDERER_H
#define USB_REMOTE_RENDERER_H

#include <stdint.h>
#include <stdbool.h>

/** \file
  * \brief Remote rendering via USB
  * \details The remote renderer does not conform to the renderer_t interface since it immediately
  *   pushes the received frames to the frame queue to reduce display latency.
  */

/// State of the current remote frame transfer.
struct remote_transfer_t {
  uint8_t* buffer_pos; ///< Current write position in the buffer.
  uint16_t buffer_remaining; ///< Remaining number of bytes to be transfered.
};

/// Free resources associated with the remote frame transfers.
void remote_renderer_stop();

/** Get the current remote frame transfer state.
  * If the returned pointer is `NULL`, this indicates that no display frame buffer could
  * be allocated and the remote communications should be halted.
  */
struct remote_transfer_t* remote_renderer_get_current();

/** Submit the current frame to the frame queue.
  * Returns false if the transfer was incomplete or the frame can't be submitted to the queue.
  */
bool remote_renderer_finish();

#endif //USB_REMOTE_RENDERER_H
