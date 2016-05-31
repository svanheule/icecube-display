#ifndef USB_DESCRIPTOR_H
#define USB_DESCRIPTOR_H

/** \file
  * \brief USB descriptor definitions and handling.
  * \details In the USB protocol descriptors are a vital part of device enumeration. It is a
  *   flexible and extensive system, but requires some effort from the developer to implement
  *   properly. The constants, structs and functions provided here attempt to make adding new
  *   descriptors as painless as possible. Most structs defined in this file use the notation
  *   from the USB specification instead of notation used elsewhere in this project for easier
  *   referencing.
  *
  *   When a setup request response is generated using generate_descriptor_list(), a linked list
  *   with the required descriptors will be dynamically allocated. Since USB descriptors are
  *   usually only requested during device enumeration, the leaves the memory free to be used
  *   for other purposes during USB operation, such as remote display frame transfers.
  * \author Sander Vanheule (Universiteit Gent)
  * \see [USB 2.0 specification §9](http://www.usb.org/developers/docs/usb20_docs/)
  */

#include <stdint.h>
#include <stdbool.h>

#include "usb/std.h"

/// List of recognised USB descriptors. Note that not all descriptors are required
/// for full-speed-only devices.
enum usb_descriptor_type_t {
    DESC_TYPE_DEVICE = 1
  , DESC_TYPE_CONFIGURATION = 2
  , DESC_TYPE_STRING = 3
  , DESC_TYPE_INTERFACE = 4
  , DESC_TYPE_ENDPOINT = 5
  , DESC_TYPE_DEVICE_QUALIFIER = 6 // High-speed USB descriptor, not used
  , DESC_TYPE_OTHER_SPEED_CONFIGURATION = 7 // High-speed USB, not used
  , DESC_TYPE_INTERFACE_POWER = 8 // USB power delivery, not used
};

/// Define which memory space the body of a USB descriptor is in.
enum memspace_t {
    MEMSPACE_RAM
  , MEMSPACE_PROGMEM
  , MEMSPACE_EEPROM
};

/// Common header used for all USB descriptors.
struct usb_descriptor_header_t {
  uint8_t bLength; ///< Total byte size of the descriptor.
  uint8_t bDescriptorType; ///< Type of the descriptor; see ::usb_descriptor_type_t.
};

/** \brief Calculate the total size of a USB descriptor
  * \details This function performs the same calculation as usb_descriptor_body_size(), but adds
  *   the size of the descriptor header that is identical for all descriptors.
  * \returns The total size in bytes of a USB descriptor.
  */
uint8_t usb_descriptor_size(
    const enum usb_descriptor_type_t type
  , const void* body
  , const enum memspace_t memspace
);

/** \brief Calculate the size of a USB descriptor's body.
  * \details Most descriptors have a predifined length, so in those cases this function just
  *   returns a constant. String descriptors however have no predetermined length, so their
  *   length needs to be determined at runtime.
  * \param type Type of the descriptor.
  * \param body Pointer to the body of the descriptor.
  * \param memspace Memory space the descriptor body resides in.
  * \returns The total size in bytes of a USB descriptor's body.
  */
uint8_t usb_descriptor_body_size(
    const enum usb_descriptor_type_t type
  , const void* body
  , const enum memspace_t memspace
);

/** \brief Single item of a linked list of descriptors.
  * \details Although descriptor list lengths are very likely to be constant throughout the
  *   runtime of the firmware, lists are allocated at runtime. To be able to allocate variable
  *   length lists, items are allocated one at a time.
  *   #next is used to refer to the next item in the list.
  *   If this is the last item in the list, #next will be equal to NULL.
  */
struct descriptor_list_t {
  struct usb_descriptor_header_t header; ///< Descriptor header
  enum memspace_t memspace; ///< Memory space the descriptor body resides in.
  const void* body; ///< Pointer to the descriptor body.
  struct descriptor_list_t* next; ///< Pointer to the next item or NULL if this is the last one.
};

/** Create a linked list of USB descriptors based on the provided setup request.
  * \param req Pointer to the setup request packet.
  * \returns A linked list of descriptor_list_t objects.
  */
struct descriptor_list_t* generate_descriptor_list(const struct usb_setup_packet_t* req);

/** \brief Calculate the sum of all descriptor sizes in a linked list.
  * \details This function is used when a configuration descriptor is provided, which needs to
  *   contain the total lenght off all returned descriptors.
  * \param head Pointer to the first item in the list.
  * \returns Total byte size of the descriptor list.
  */
uint16_t get_list_total_length(const struct descriptor_list_t* head);

/// Device descriptor body
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

/// Configuration descriptor body
struct usb_descriptor_body_configuration_t {
  uint16_t wTotalLength;
  uint8_t bNumInterfaces;
  uint8_t bConfigurationValue;
  uint8_t iConfiguration;
  uint8_t bmAttributes;
  uint8_t bMaxPower;
};

/// Interface descriptor body
struct usb_descriptor_body_interface_t {
  uint8_t bInterfaceNumber;
  uint8_t bAlternateSetting;
  uint8_t bNumEndPoints;
  uint8_t bInterfaceClass;
  uint8_t bInterfaceSubClass;
  uint8_t bInterfaceProtocol;
  uint8_t iInterface;
};

/// Endpoint descriptor body
struct usb_descriptor_body_endpoint_t {
  uint8_t bEndpointAddress;
  uint8_t bmAttributes;
  uint16_t wMaxPacketSize;
  uint8_t bInterval;
};

#endif
