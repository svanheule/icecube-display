#ifndef USB_REMOTE_RENDERER_H
#define USB_REMOTE_RENDERER_H

#include <stdint.h>
#include <stdbool.h>

/** \file
  * \brief Remote rendering via USB
  * \details The remote renderer does not conform to the ::renderer_t interface since it immediately
  *   pushes the received frames to the frame queue to reduce display latency.
  * \author Sander Vanheule (Universiteit Gent)
  */

/** \page usb_remote_renderer Frame data transfer over USB
  * The default USB device configuration (configuration 1) provides a bulk OUT endpoint to
  * transmit frame buffer data from the controlling PC to the display.
  * After selecting configuration 1 with ::SET_CONFIGURATION the internal state of the endpoint
  * is reset.
  * The frame data can then be written to EP1 without any header or associated control request.
  *
  * Since the endpoint size is smaller than the frame buffer size, multiple USB transfers have to
  * performed to complete a single frame buffer transfer.
  * A USB transfer with a size smaller than the endpoint size is *always* considered to indicate
  * the last transfer of the series.
  * If not enough data has been received to entirely fill a frame buffer, the frame transfer is
  * considered invalid, the frame buffer is discarded, and the endpoint is stalled to notify the
  * host of the transfer failure.
  *
  * If the display frame buffer size is an exact multiple of the endpoint size, the transfer is
  * considered complete when enough data has been received to fill the frame buffer.
  * Note that it is not required to transmit an extra zero-length USB frame to complete the
  * transfer.
  *
  * If the endpoint was stalled due to a transmission error, the stall should be clear using a
  * \ref CLEAR_FEATURE control request.
  *
  * \dot
  *   digraph remote_renderer_fsm {
  *     rankdir=LR;
  *     node [shape=circle];
  *     edge [fontsize=12];
  *     stall [style=filled, fillcolor=orange, label="STALL"];
  *     start [shape=point];
  *     transfer [shape=doublecircle, label="TRANSFER"];
  *     submit [label="SUBMIT\nFRAME"]
  *
  *     start -> transfer;
  *     transfer -> transfer [label="remaining != 0 && \n transfer_size == endpoint_size"];
  *     transfer -> stall [label="remaining != 0 && \n transfer_size < endpoint_size"];
  *     transfer -> submit [label="remaining == 0"];
  *     submit -> transfer;
  *   }
  * \enddot
  */

/** \defgroup led_display_remote Remote rendering
  * \ingroup led_display
  * \brief Remote rendering of display content.
  * \see \ref usb_remote_renderer
  * @{
  */

/// State of the current remote frame transfer.
struct frame_transfer_state_t {
  uint8_t* write_pos;
  uint8_t* buffer_end;
};

/// Initialise the remote renderer internal state by acquiring a frame buffer.
void remote_renderer_init();

/// Stall the remote renderer's endpoint and free frame buffer resources.
void remote_renderer_halt();

/// Free resources associated with the remote frame transfers. Does not stall the endpoint.
void remote_renderer_stop();

/** Get the current remote frame transfer state.
  * If the state pointers are `NULL`, this indicates that no display frame buffer could
  * be allocated and the remote communications should be halted.
  */
struct frame_transfer_state_t* remote_renderer_get_transfer_state();

/** Submit the current frame to the frame queue.
  * Will halt the endpoint if no room was available in the frame queue.
  */
void remote_renderer_transfer_done();

/// @}

#endif //USB_REMOTE_RENDERER_H
