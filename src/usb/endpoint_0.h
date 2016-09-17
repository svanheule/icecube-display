#ifndef USB_ENDPOINT_0_H
#define USB_ENDPOINT_0_H

/** \defgroup usb_endpoint_control Default control endpoint
  * \ingroup usb_endpoint
  * \brief Default control endpoint (endpoint 0) behaviour.
  * \details Control transfers are handled entirely asynchronously via interrupts. When the device
  *   has to wait for an answer from the host for example, this means that the firmware won't wait
  *   and poll to see if an answer has been received, but resumes normal operation until an
  *   interrupt is fired. This implies the use of task specific callbacks, since different setup
  *   request require different actions to be performed, possibly at different points in the
  *   request's lifetime.
  *
  *   The control endpoint process requests in the following way, as illustrated by the diagram
  *   below:
  *   - Initially, the control endpoint's FSM is in the ::CTRL_IDLE state and transitions to
  *     the ::CTRL_SETUP state if a new setup packet has been received.
  *   - If the direction is IN, the endpoint prepares the data for transmission and enters the
  *     ::CTRL_DATA_IN state. Once this data is transmitted, the FSM proceeds to the
  *     ::CTRL_HANDSHAKE_IN state, waiting for a zero-length packet (ZLP) confirming the data was
  *     correctly received. This is then followed by possible a post-handshake action.
  *     and state if needed. The FSM then transitions back to the ::CTRL_IDLE state, waiting
  *     for a new setup request.
  *   - If the direction of the request is OUT, it is possible no extra data except for the request
  *     will be provided by the host. In this case, the FSM transitions to ::CTRL_HANDSHAKE_OUT to
  *     acknowledge the transaction. If extra data will be sent, the FSM first enters the
  *     ::CTRL_DATA_OUT state, waiting to receive more data.
  *   - If an unknown or bad request is received, or the device is unable
  *     handle the requested/provided data, the FSM will enter the ::CTRL_STALL state and
  *     stall the endpoint, notifying the host of a failed transaction.
  *     The stall is automatically cleared once the next setup request is received.
  *
  *   \dot
  *     digraph setup_fsm {
  *       node [shape=record];
  *       idle [shape=doublecircle, label=IDLE, URL="\ref ::CTRL_IDLE"];
  *       stall [style=filled, fillcolor=orange, label="STALL", URL="\ref ::CTRL_STALL"];
  *       setup [label=SETUP, URL="\ref ::CTRL_SETUP"];
  *       data_in [label="{DATA_IN | Send IN frame(s) with data}", URL="\ref ::CTRL_DATA_IN"];
  *       data_out [
  *           label="{DATA_OUT | Receive OUT frame(s) with data}"
  *         , URL="\ref ::CTRL_DATA_OUT"
  *       ];
  *       handshake_in [
  *           label="{HANDSHAKE_IN | Wait for ZLP OUT frame}"
  *         , URL="\ref ::CTRL_HANDSHAKE_IN"
  *       ];
  *       handshake_out [
  *           label="{HANDSHAKE_OUT | Send ZLP IN frame}"
  *         , URL="\ref ::CTRL_HANDSHAKE_OUT"
  *       ];
  *       post_handshake [
  *           label="{POST_HANDSHAKE | Post-ZLP action}"
  *          , URL="\ref ::CTRL_POST_HANDSHAKE"
  *       ];
  *       {rank=same stall idle} -> setup;
  *       subgraph data_transmission {
  *         setup -> {data_in data_out handshake_out};
  *         data_in -> handshake_in -> post_handshake;
  *         data_out -> handshake_out -> post_handshake;
  *       }
  *       {handshake_in handshake_out post_handshake} -> idle;
  *     }
  *   \enddot
  */

#include <stdint.h>
#include "usb/std.h"

/// \brief Constants used to indicate the default control endpoint's current state.
/// \ingroup usb_endpoint_control
enum control_stage_t {
    CTRL_IDLE ///< Control endpoint idle
  , CTRL_SETUP ///< New setup request received
  , CTRL_STALL ///< Control endpoint stalled
  , CTRL_DATA_IN ///< Sending data to host
  , CTRL_DATA_OUT ///< Receiving data from host
  , CTRL_HANDSHAKE_IN ///< Handshaking IN transfer
  , CTRL_HANDSHAKE_OUT ///< Handshaking OUT transfer
  , CTRL_POST_HANDSHAKE ///< Performing post-handshake action
};

/// \brief Control transfer state tracking.
/// \ingroup usb_endpoint_control
struct control_transfer_t {
  /// Stage the request is currently in.
  enum control_stage_t stage;
  /// Setup request that is currently being handled.
  const struct usb_setup_packet_t* req;
  /// Data to be transmitted to the host.
  void* data_in;
  /// Total length of \a data_in.
  uint16_t data_in_length;
  /// Transmitted length of \a data_in.
  uint16_t data_in_done;
  /// Function to be called when new data is received.
  void (*callback_data_out)(struct control_transfer_t* transfer);
  /// Function to be called *after* the ZLP handshake is sent/received.
  void (*callback_handshake)(struct control_transfer_t* transfer);
  /// Cleanup function to be called when the transfer is cancelled.
  void (*callback_cancel)();
};

/** \brief Process setup request for control transfers
  * \details The \a transfer object is updated with the correct information to
  *   continue processing the control transfer. In case this is a valid and
  *   recognised setup packet, `transfer->stage` will be updated to the state
  *   the transfer is in once this function returns.
  *   If the control transfer stage is still `CTRL_SETUP` when this function
  *   returns, the request should be considered invalid and the control endpoint
  *   stalled.
  * \ingroup usb_endpoint_control 
  */
void process_setup(struct control_transfer_t* transfer);

void cancel_control_transfer(struct control_transfer_t* transfer);

#endif //USB_ENDPOINT_0_H

