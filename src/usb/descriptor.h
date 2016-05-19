#ifndef USB_DESCRIPTOR_H
#define USB_DESCRIPTOR_H

#include <stdint.h>
#include <stdbool.h>

#include "usb/std.h"

enum usb_descriptor_type_t {
    DESC_TYPE_DEVICE = 1
  , DESC_TYPE_CONFIGURATION = 2
  , DESC_TYPE_STRING = 3
  , DESC_TYPE_INTERFACE = 4
  , DESC_TYPE_ENDPOINT = 5
  , DESC_TYPE_DEVICE_QUALIFIER = 6 // High-speed USB, not used
  , DESC_TYPE_OTHER_SPEED_CONFIGURATION = 7 // High-speed USB, not used
  , DESC_TYPE_INTERFACE_POWER = 8 // USB power delivery, not used
};

enum memspace_t {
    MEMSPACE_RAM
  , MEMSPACE_PROGMEM
  , MEMSPACE_EEPROM
};

// Common header used for all USB descriptors
struct usb_descriptor_header_t {
  uint8_t bLength;
  uint8_t bDescriptorType;
};

#define HEADER_SIZE sizeof(struct usb_descriptor_header_t)

struct usb_descriptor_t {
  enum usb_descriptor_type_t type;
  void* body;
};

uint8_t usb_descriptor_size(const enum usb_descriptor_type_t type, const void* body, const enum memspace_t memspace);
uint8_t usb_descriptor_body_size(const enum usb_descriptor_type_t type, const void* body, const enum memspace_t memspace);

struct descriptor_list_t {
  struct usb_descriptor_header_t header;
  enum memspace_t memspace;
  const void* body;
  struct descriptor_list_t* next;
};

struct descriptor_list_t* generate_descriptor_list(const struct usb_setup_packet_t* req);
uint16_t get_list_total_length(const struct descriptor_list_t* head);

// Device descriptor definitions
struct usb_descriptor_body_device_t {
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
};

struct usb_decriptor_device_t {
  struct usb_descriptor_header_t header;
  struct usb_descriptor_body_device_t body;
};

// Configuration descriptor definitions
struct usb_descriptor_body_configuration_t {
  uint16_t wTotalLength;
  uint8_t bNumInterfaces;
  uint8_t bConfigurationValue;
  uint8_t iConfiguration;
  uint8_t bmAttributes;
  uint8_t bMaxPower;
};

struct usb_descriptor_configuration_t {
  struct usb_descriptor_header_t header;
  struct usb_descriptor_body_configuration_t body;
};

// Interface descriptor definitions
struct usb_descriptor_body_interface_t {
  uint8_t bInterfaceNumber;
  uint8_t bAlternateSetting;
  uint8_t bNumEndPoints;
  uint8_t bInterfaceClass;
  uint8_t bInterfaceSubClass;
  uint8_t bInterfaceProtocol;
  uint8_t iInterface;
};

struct usb_descriptor_interface_t {
  struct usb_descriptor_header_t header;
  struct usb_descriptor_body_interface_t body;
};

// Endpoint descriptor definitions
struct usb_descriptor_body_endpoint_t {
  uint8_t bEndpointAddress;
  uint8_t bmAttributes;
  uint16_t wMaxPacketSize;
  uint8_t bInterval;
};

struct usb_descriptor_endpoint_t {
  struct usb_descriptor_header_t header;
  struct usb_descriptor_body_endpoint_t body;
};

#endif

