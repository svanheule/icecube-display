#ifndef USB_H
#define USB_H

#include <stdint.h>

#define BCD_USB_VERSION 0x0200
#define ID_VENDOR 0x0000 // FIXME Set correct VID/PID
#define ID_PRODUCT 0x0000

typedef enum PacketId_t {
    OUT = 0x1
  , IN = 0x9
  , SOF = 0x5
  , SETUP = 0xD
  , DATA0 = 0x3
  , DATA1 = 0xB
  , ACK = 0x2
  , NAK = 0xA
  , STALL = 0xE
} PacketId;

typedef enum UsbDescriptorType_t {
    DEVICE = 0
  , CONFIGURATION = 1
  , STRING = 3
  , INTERFACE = 4
  , ENDPOINT = 5
  , DEVICE_QUALIFIER = 6
  , OTHER_SPEED_CONFIGURATION = 7
  , INTERFACE_POWER = 8
} UsbDescriptorType;

typedef enum UsbRequestCode_t {
    GET_STATUS = 0
  , CLEAR_FEATURE = 1
  , SET_FEATURE = 3
  , SET_ADDRESS = 5
  , GET_DESCRIPTOR = 6
  , SET_DESCRIPTOR = 7
  , GET_CONFIGURATION = 8
  , SET_CONFIGURATION = 9
  , GET_INTERFACE = 10
  , SET_INTEFACE = 11
  , SYNCH_FRAME = 12
} UsbRequestCode;

#define REQUEST_DIRECTION_MASK (0x1<<7)
#define REQUEST_DIRECTION_OUT (0<<7)
#define REQUEST_DIRECTION_IN (1<<7)
#define GET_REQUEST_DIRECTION(bmRequestType) (bm_RequestType & REQUEST_DIRECTION_MASK)

#define REQUEST_TYPE_MASK (0x3<<5)
#define REQUEST_TYPE_STANDARD (0<<5)
#define REQUEST_TYPE_CLASS (1<<5)
#define REQUEST_TYPE_VENDOR (2<<5)
#define GET_REQUEST_TYPE(bmRequestType) (bmRequestType & REQUEST_TYPE_MASK)

#define REQUEST_RECIPIENT_MASK 0x1F
#define REQUEST_RECIPIENT_DEVICE 0
#define REQUEST_RECIPIENT_INTERFACE 1
#define REQUEST_RECIPIENT_ENDPOINT 2
#define REQUEST_RECIPIENT_OTHER 3
#define GET_REQUEST_RECIPIENT(bmRequestType) (bmRequestType & REQUEST_RECIPIENT_MASK)

#define DEVICE_STATUS_SELF_POWERED 1
#define DEVICE_STATUS_REMOTE_WAKEUP 2
#define ENDPOINT_STATUS_HALTED 1

typedef enum UsbFeature_t {
    ENDPOINT_HALT = 0
  , REMOTE_WAKEUP = 1
  , TEST_MODE = 2
} UsbFeature;


typedef struct UsbSetupPacket_t {
  uint8_t bmRequestType;
  UsbRequestCode bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
} UsbSetupPacket;

typedef struct UsbDescriptorHeader_t{
  uint8_t bLength;
  UsbDescriptorType bDescriptorType;
} UsbDescriptorHeader;

typedef struct UsbDeviceDescriptor_t {
  UsbDescriptorHeader header;
  uint16_t bcdUSB;
  uint8_t bDeviceClass;
  uint8_t bDeviceSubClass;
  uint8_t bDeviceProtocol;
  uint8_t bMaxPacketSize;
  uint16_t idVendor;
  uint16_t idProduct;
  uint16_t bcdDevice;
  uint8_t iManufacturer;
  uint8_t iProduct;
  uint8_t iSerialNumber;
  uint8_t bNumConfigurations;
} UsbDeviceDescriptor;

typedef struct UsbConfigurationDescriptor_t {
  UsbDescriptorHeader header;
  uint16_t wTotalLength;
  uint8_t bNumInterfaces;
  uint8_t bConfigurationValue;
  uint8_t iConfiguration;
  uint8_t bmAttributes;
  uint8_t bMaxPower;
} UsbConfigurationDescriptor;

typedef struct UsbInterfaceDescriptor_t {
  UsbDescriptorHeader header;
  uint8_t bInterfaceNumber;
  uint8_t bAlternateSetting;
  uint8_t bNumEndPoints;
  uint8_t bInterfaceClass;
  uint8_t bInterfaceSubClass;
  uint8_t bInterfaceProtocol;
  uint8_t iInterface;
} UsbInterfaceDescriptor;

typedef struct UsbEndpointDescriptor_t {
  UsbDescriptorHeader header;
  uint8_t bEndpointAddress;
  uint8_t bmAttributes;
  uint16_t wMaxPacketSize;
  uint8_t bInterval;
} UsbEndpointDescriptor;

typedef __CHAR16_TYPE__ char16_t;

typedef struct UsbStringDescriptor_t {
  UsbDescriptorHeader header;
  union {
    char16_t* bString;
    uint16_t* wLangId;
  };
} UsbStringDescriptor;

#endif // USB_H
