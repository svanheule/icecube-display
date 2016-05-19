#ifndef USB_STD_H
#define USB_STD_H

#include <stdint.h>

enum usb_request_code_t {
    GET_STATUS = 0
  , CLEAR_FEATURE = 1
  , SET_FEATURE = 3
  , SET_ADDRESS = 5
  , GET_DESCRIPTOR = 6
  , SET_DESCRIPTOR = 7
  , GET_CONFIGURATION = 8
  , SET_CONFIGURATION = 9
  , GET_INTERFACE = 10
  , SET_INTERFACE = 11
  , SYNCH_FRAME = 12
};

#define REQ_DIR_MASK (0x1<<7)
#define REQ_DIR_OUT (0<<7)
#define REQ_DIR_IN (1<<7)
#define GET_REQUEST_DIRECTION(bmRequestType) (bmRequestType & REQ_DIR_MASK)

#define REQ_TYPE_MASK (0x3<<5)
#define REQ_TYPE_STANDARD (0<<5)
#define REQ_TYPE_CLASS (1<<5)
#define REQ_TYPE_VENDOR (2<<5)
#define GET_REQUEST_TYPE(bmRequestType) (bmRequestType & REQ_TYPE_MASK)

#define REQ_REC_MASK 0x1F
#define REQ_REC_DEVICE 0
#define REQ_REC_INTERFACE 1
#define REQ_REC_ENDPOINT 2
#define REQ_REC_OTHER 3
#define GET_REQUEST_RECIPIENT(bmRequestType) (bmRequestType & REQ_REC_MASK)

#define DEVICE_STATUS_SELF_POWERED 1
#define DEVICE_STATUS_REMOTE_WAKEUP 2
#define ENDPOINT_STATUS_HALTED 1

enum usb_feature_t {
    ENDPOINT_HALT = 0
  , REMOTE_WAKEUP = 1
  , TEST_MODE = 2
};


struct usb_setup_packet_t {
  uint8_t bmRequestType;
  uint8_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
};

#endif // USB_STD_H
