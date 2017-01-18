#ifndef USB_ENDPOINT_0_H
#define USB_ENDPOINT_0_H

/** \file
  * \brief USB Endpoint 0 state machine interface.
  * \details
  *   Control transfers are usually handled asynchronously via interrupts. When the device
  *   has to wait for an answer from the host for example, this means that the firmware won't wait
  *   and poll to see if an answer has been received, but resumes normal operation until an
  *   interrupt is fired. This implies the use of task specific callbacks, since different setup
  *   request require different actions to be performed, possibly at different points in the
  *   request's lifetime.
  *
  * \see \ref usb_endpoint_control
  *
  * \see See section 9.3 of the
  * [USB 2.0 specification](http://www.usb.org/developers/docs/usb20_docs/) for more details.
  *
  * \author Sander Vanheule (Universiteit Gent)
  */

/** \page usb_endpoint_control Default control endpoint
  * ## Supported requests
  * Aside from the \ref ::usb_request_code_t "standard request" support required by
  * the USB standard for proper functioning of the device, a number of
  * \ref ::vendor_request_t "vendor specific requests" can also be handled.
  *
  * ## Request handling
  * The control endpoint processes requests in the following way, as illustrated by the diagram
  * below:
  * * Initially, the control endpoint's FSM is in the ::CTRL_IDLE state.
  * * When a new setup packet is received, it should be copied into a usb_setup_packet_t object.
  *   This should then be passed, together with a (new) control_transfer_t object, to
  *   init_control_transfer().
  * * After the control_transfer_t object is properly initialised, it should be passed to
  *   process_setup(). The control_transfer_t object will now be in the ::CTRL_SETUP state.
  * * Pass the initialised control transfer object to process_setup(). If the state did not follow
  *   one of the allowed transitions from ::CTRL_SETUP, then this is a bad request and the endpoint
  *   should be stalled to notify the host.
  *   The stall is automatically cleared once the next setup request is received.
  * * Depending on the nature of the transfers, three valid states are now possible:
  *   * The data direction is IN, so the data for transmission is made available and the control
  *     transfer enters the ::CTRL_DATA_IN state.
  *   * The direction of the request is OUT, and additional data will follow.
  *     The FSM transitions to ::CTRL_DATA_OUT.
  *   * The direction of the request is OUT, but the request will have no data stage.
  *     In this case, the FSM transitions to ::CTRL_HANDSHAKE_OUT to acknowledge the transaction.
  * * If there is a data stage (::CTRL_DATA_IN or ::CTRL_DATA_OUT), data has to be transferred
  *   from the host to the device or vice-versa.
  *   Since the data size is potentially larger than the endpoint size, this may mean multiple
  *   transfers will take place.
  *   Every time data is succesfully transfered, control_mark_data_done() should be called.
  *   This will increment the data pointer by the transferred amount and perform the data stage
  *   callback.
  * * Once all data is transferred, there is a compulsory handshake phase.
  *   For the ::CTRL_HANDSHAKE_IN state, this means waiting for a zero-length packet (ZLP) from
  *   the host confirming all the data from the device was correctly received.
  *   For the ::CTRL_HANDSHAKE_OUT state, this means sending a ZLP to indicate all data was
  *   received by the device.
  *   This is then followed by possible a post-handshake action and state if needed.
  *   The FSM should then be put back in the ::CTRL_IDLE state, waiting for a new setup request.
  *
  * \dot
  *   digraph setup_fsm {
  *     node [shape=record];
  *     idle [shape=doublecircle, label=IDLE, URL="\ref ::CTRL_IDLE"];
  *     stall [style=filled, fillcolor=orange, label="STALL", URL="\ref ::CTRL_STALL"];
  *     setup [label=SETUP, URL="\ref ::CTRL_SETUP"];
  *     data_in [label="{DATA_IN | Send IN frame(s) with data}", URL="\ref ::CTRL_DATA_IN"];
  *     data_out [
  *         label="{DATA_OUT | Receive OUT frame(s) with data}"
  *       , URL="\ref ::CTRL_DATA_OUT"
  *     ];
  *     handshake_in [
  *         label="{HANDSHAKE_IN | Wait for ZLP OUT frame}"
  *       , URL="\ref ::CTRL_HANDSHAKE_IN"
  *     ];
  *     handshake_out [
  *         label="{HANDSHAKE_OUT | Send ZLP IN frame}"
  *       , URL="\ref ::CTRL_HANDSHAKE_OUT"
  *     ];
  *     post_handshake [
  *         label="{POST_HANDSHAKE | Post-ZLP action}"
  *        , URL="\ref ::CTRL_POST_HANDSHAKE"
  *     ];
  *     finish [shape=circle, label="done"];
  *     {rank=same stall idle} -> setup;
  *     subgraph data_transmission {
  *       setup -> {data_in data_out handshake_out};
  *       data_in -> handshake_in -> post_handshake;
  *       data_out -> handshake_out -> post_handshake;
  *     }
  *     {handshake_in handshake_out post_handshake} -> finish;
  *   }
  * \enddot
  */

#include <stdint.h>
#include "usb/std.h"

/// \brief Constants used to indicate the default control endpoint's current state.
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

/** Vendor specific USB control request for display status and control.
  * Request name                        | bmRequestType | bRequest | wValue | wIndex |    wLength
  * ------------------------------------|---------------|----------|--------|--------|-----------
  * ::VENDOR_REQUEST_DISPLAY_PROPERTIES |  0b1_10_00000 |        2 |      0 |      0 |    2-65535
  * ::VENDOR_REQUEST_EEPROM_WRITE       |  0b0_10_00000 |        3 |      0 | offset |     length
  * ::VENDOR_REQUEST_EEPROM_READ        |  0b1_10_00000 |        4 |      0 | offset |     length
  * ::VENDOR_REQUEST_FRAME_DRAW_STATUS  |  0b1_10_00000 |        5 |      0 |      0 |          4
  * ::VENDOR_REQUEST_FRAME_DRAW_SYNC    |  0b0_10_00000 |        6 |   [ms] |      0 |          0
  * \see \ref usb_endpoint_control
  */
enum vendor_request_t {
  /** Push a single frame to be shown on the display into the frame buffer queue.
    * When connected via USB all other renderers are stopped, so this frame will be
    * displayed until the next one is pushed.
    * Note that the device only updates the display 25 times per second, so pushing frame more
    * frequently than this will result in buffer overflows on the device and consequently control
    * transfers will be stalled until memory is freed.
    * Since some platforms may not support control transfers of the size needed to send a frame,
    * it is possible to send a single frame using multiple *PUSH_FRAME* commands.
    * \deprecated Use the bulk out endpoint EP1 to transmit frame data.
    */
  VENDOR_REQUEST_PUSH_FRAME = 1,
  /** Request a TLV list of [display metadata](\ref display_metadata).
    * A reply to this request will always consist of at least two bytes,
    * which provide the total length of the response.
    * A typical query of this metadata will be done the following way:
    *   1. Perform an IN transfer of wLength 2. This will return the full length of
    *      the TLV data as an unsigned 16 bit, little endian integer.
    *   2. Perform an IN transfer of wLength N, with N being the response of the first request.
    */
  VENDOR_REQUEST_DISPLAY_PROPERTIES = 2,
  /** To perform the (initial) configuration of the device, one may also use the USB interface
    *  to read to and write from the microcontroller's EEPROM.
    * *EEPROM_READ* and *EEPROM_WRITE* allow access to an EEPROM segment of arbitrary length,
    * starting from any offset address that is within the size of the EEPROM.
    *
    * The wIndex field of the setup request is used to provide the address offset of the EEPROM
    * memory segment that is to be written.
    * A request with `wIndex=0x00` will start writing at the first byte, while a request
    * with `wIndex=0x30` will start writing at the 49th byte.
    * The wLength field provides the length of the EEPROM segment that is to be written.
    * If either wIndex or wIndex+wLength is larger than the size of the device's EEPROM, the
    * control endpoint will be stalled indicating a bad request.
    *
    * Use the *EEPROM_WRITE* command with care, as writing bad data to the EEPROM may render the
    * device unusable.
    */
  VENDOR_REQUEST_EEPROM_WRITE = 3,
  /** Read a segment from the device's EEPROM. See ::VENDOR_REQUEST_EEPROM_WRITE on how to use
    * the wIndex and wLength fields.
    * For example, the IceCube string to buffer offset mapping of IceCube display microcontrollers
    * can be read using `wIndex=0x30` and `wLength=36`.
    */
  VENDOR_REQUEST_EEPROM_READ = 4,
  /** Get the latest frame draw time.
    * The request response consists of two unsigned 16 bit (little endian) integers a defined
    * by ::display_frame_usb_phase_t.
    */
  VENDOR_REQUEST_FRAME_DRAW_STATUS = 5,
  /** Correct the frame counter ms value by the provided amount.
    * The ms correction \f$\delta\f$ is provided as a _signed_ (little endian) 16 bit integer
    * contained in the wValue field of the setup request.
    * Corrections larger than \f$\pm(2^{15}-1)\f$ms should be performed using multiple requests.
    * In this case the first request should perform the largest possible correction including
    * the full USB frame counter phase slip, i.e.
    * \f$(\delta \bmod 40) + N \times 40 = \Delta_0\f$ with \f$|\Delta_0| \le 2^{15}-1\f$.
    * This synchronises the fraw draws down to the millisecond, but leaves the frame counters out
    * of phase.
    * Subsequent requests can then correct the remaining offset as multiples of 40ms.
    */
  VENDOR_REQUEST_FRAME_DRAW_SYNC = 6
};

/// \brief Control transfer state tracking.
struct control_transfer_t {
  /// Stage the request is currently in.
  enum control_stage_t stage;
  /// Setup request that is currently being handled.
  const struct usb_setup_packet_t* req;
  /// Data to be transmitted to/from the host.
  void* data;
  /// Total length of \a data.
  uint16_t data_length;
  /// Transmitted length of \a data.
  uint16_t data_done;
  /// Function to be called when new data is received or sent.
  /// When the transfer is completed, this callback should move the transfer
  /// into the handshake phase.
  void (*callback_data)(struct control_transfer_t* transfer);
  /// Function to be called *after* the ZLP handshake is sent/received.
  void (*callback_handshake)(struct control_transfer_t* transfer);
  /// Cleanup function to be called when the transfer is cancelled.
  void (*callback_cancel)(struct control_transfer_t* transfer);
};

/** \brief Process setup request for control transfers
  * \details The \a transfer object is updated with the correct information to
  *   continue processing the control transfer. In case this is a valid and
  *   recognised setup packet, `transfer->stage` will be updated to the state
  *   the transfer is in once this function returns.
  *   If the control transfer stage is still `CTRL_SETUP` when this function
  *   returns, the request should be considered invalid and the control endpoint
  *   stalled.
  */
void process_setup(struct control_transfer_t* transfer);

/** \brief Initialise the control transfer.
  * \details Sets the transfer stage to SETUP and stores the provided
  *   setup packet \a setup as the corresping request for the transfer.
  */
void init_control_transfer(
      struct control_transfer_t* transfer
    , const struct usb_setup_packet_t* setup
);

/// Change the transfer state to STALL and perform the cancel callback, if any.
void cancel_control_transfer(struct control_transfer_t* transfer);

/// Mark \a length bytes of the control transfer as done and perform the data callback.
void control_mark_data_done(struct control_transfer_t* transfer, uint16_t length);

#endif //USB_ENDPOINT_0_H

