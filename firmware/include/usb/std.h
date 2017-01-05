#ifndef USB_STD_H
#define USB_STD_H

/** \file
  * \brief Definitions of constants as described by the USB 2.0 standard.
  * \details A few types, constants and macros used by the USB subsystem.
  * \author Sander Vanheule (Universiteit Gent)
  * \see [USB 2.0 specification](http://www.usb.org/developers/docs/usb20_docs/)
  */

#include <stdint.h>

/// USB packet IDs
/// \ingroup usb_endpoint
enum usb_pid_t {
    PID_OUT = 0x1
  , PID_IN = 0x9
  , PID_SOF = 0x5
  , PID_SETUP = 0xD
  , PID_DATA0 = 0x3
  , PID_DATA1 = 0xB
  , PID_DATA2 = 0x7
  , PID_MDATA = 0xF
  , PID_ACK = 0x2
  , PID_NAK = 0xA
  , PID_STALL = 0xE
  , PIT_NYET = 0x6
  , PID_PRE = 0xC
  , PID_ERR = 0xC
  , PID_SPLIT = 0x8
  , PID_PING = 0x4
};

/// USB request codes defined in §9.4 of the specification.
/// Not all requests are currently supported.
/// \ingroup usb_endpoint_control
enum usb_request_code_t {
    GET_STATUS = 0 ///< Get device or endpoint status.
  , CLEAR_FEATURE = 1 ///< Turn device or endpoint feature off. See ::usb_feature_t
  , SET_FEATURE = 3 ///< Turn device or endpoint feature on. See ::usb_feature_t
  , SET_ADDRESS = 5 ///< Set device address
  , GET_DESCRIPTOR = 6 ///< Get USB descriptor
  , SET_DESCRIPTOR = 7 ///< Set USB descriptor, not supported
  , GET_CONFIGURATION = 8 ///< Get currently selected configuration index.
  , SET_CONFIGURATION = 9 ///< Set device configuration.
  , GET_INTERFACE = 10 ///< Get currently selected interface, not supported
  , SET_INTERFACE = 11 ///< Select interface, not supported.
  , SYNCH_FRAME = 12 ///< Device synchronisation for isochronous configurations, not supported.
};

/// \name USB request direction
/// \ingroup usb_endpoint_control
/// @{
#define REQ_DIR_MASK (0x1<<7)
#define REQ_DIR_OUT (0<<7)
#define REQ_DIR_IN (1<<7)
#define GET_REQUEST_DIRECTION(bmRequestType) (bmRequestType & REQ_DIR_MASK)
/// @}

/// \name USB request type
/// \ingroup usb_endpoint_control
/// @{
#define REQ_TYPE_MASK (0x3<<5)
#define REQ_TYPE_STANDARD (0<<5)
#define REQ_TYPE_CLASS (1<<5)
#define REQ_TYPE_VENDOR (2<<5)
#define GET_REQUEST_TYPE(bmRequestType) (bmRequestType & REQ_TYPE_MASK)
/// @}

/// \name USB request recipient
/// \ingroup usb_endpoint_control
/// @{
#define REQ_REC_MASK 0x1F
#define REQ_REC_DEVICE 0
#define REQ_REC_INTERFACE 1
#define REQ_REC_ENDPOINT 2
#define REQ_REC_OTHER 3
#define GET_REQUEST_RECIPIENT(bmRequestType) (bmRequestType & REQ_REC_MASK)
/// @}

/// \name USB device status
/// \ingroup usb_endpoint_control
/// @{
#define DEVICE_STATUS_SELF_POWERED 1
#define DEVICE_STATUS_REMOTE_WAKEUP 2
#define ENDPOINT_STATUS_HALTED 1
/// @}

/// USB device or endpoint features.
/// \ingroup usb_endpoint_control
enum usb_feature_t {
  /// If the endpoint is/should be stalled.
  ENDPOINT_HALT = 0,
  /// Remote wake-up of host by device.
  REMOTE_WAKEUP = 1,
  /// Switch the device to test mode -- a high-speed feature, so not supported.
  TEST_MODE = 2
};


/// USB control endpoint setup packet
/// \ingroup usb_endpoint_control
struct usb_setup_packet_t {
  /// bitwise OR of a `REQ_DIR_*`, a `REQ_TYPE_*`, and a `REQ_REC_*` constant. See std.h.
  uint8_t bmRequestType;
  uint8_t bRequest; ///< Request code, interpretation depends on value of bmRequestType.
  uint16_t wValue; ///< Request value, interpretation is specific to bRequest's value.
  uint16_t wIndex; ///< Request index, interpretation is specific to bRequest's value
  uint16_t wLength; ///< Data length of the request.
} __attribute__((packed));

#endif // USB_STD_H
