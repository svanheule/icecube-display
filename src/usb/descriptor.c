#include "usb/descriptor.h"
#include <stdlib.h>
#include <avr/pgmspace.h>

typedef __CHAR16_TYPE__ char16_t;

// Return the number of 16b words in str.
// Codepoints that are split over two UTF16 char16_t values, add 2 the returned length.
static size_t strlen16(const char16_t* str) {
  const char16_t* end = str;
  while (*end != 0) {
    ++end;
  }
  return end-str;
}

static size_t strlen16_P(const char16_t* str) {
  const char16_t* end = str;
  while (pgm_read_word(end) != 0) {
    ++end;
  }
  return end-str;
}

typedef struct usb_descriptor_header_t header;
typedef struct usb_descriptor_body_device_t body_device;
typedef struct usb_descriptor_body_configuration_t body_configuration;
typedef struct usb_descriptor_body_interface_t body_interface;
typedef struct usb_descriptor_body_endpoint_t body_endpoint;

uint8_t usb_descriptor_size(
    const enum usb_descriptor_type_t type
  , const void* body
  , const bool body_in_flash
) {
  return usb_descriptor_body_size(type, body, body_in_flash) + sizeof(header);
}

uint8_t usb_descriptor_body_size(
    const enum usb_descriptor_type_t type
  , const void* body
  , const bool body_in_flash
) {
  switch (type) {
    case DESC_TYPE_CONFIGURATION:
      return sizeof(body_configuration);
      break;
    case DESC_TYPE_DEVICE:
      return sizeof(body_device);
      break;
    case DESC_TYPE_STRING:
      if (!body_in_flash) {
        return 2*strlen16((const char16_t*) body);
      }
      else {
        return 2*strlen16_P((const char16_t*) body);
      }
      break;
    case DESC_TYPE_INTERFACE:
      return sizeof(body_interface);
      break;
    case DESC_TYPE_ENDPOINT:
      return sizeof(body_endpoint);
      break;
    default:
      return 0;
      break;
  }
}

static struct descriptor_list_t* create_list_item(
    const enum usb_descriptor_type_t type
  , const void* body
  , const bool body_in_flash
) {
  typedef struct descriptor_list_t item_t;
  item_t* item = (item_t*) malloc(sizeof(item_t));
  if (item) {
    item->header.bLength = usb_descriptor_size(type, body, body_in_flash);
    item->header.bDescriptorType = type;
    item->body_in_flash = body_in_flash;
    item->body = body;
    item->next = 0;
  }
  return item;
}


// Descriptor transaction definitions
static const struct usb_descriptor_body_device_t BODY_DEVICE PROGMEM = {
    0x0200
  , 0
  , 0
  , 0
  , 64
  , 0x1ce3
  , 0x0001
  , 0x0010
  , 1
  , 2
  , 3
  , 1
};

static const char16_t STR_MANUFACTURER[] PROGMEM = u"Ghent University";
static const char16_t STR_PRODUCT[] PROGMEM = u"IceTop event display";
static const char16_t STR_SERIAL_NUMBER[] PROGMEM = u"ICD-IT-001-0001";

static const uint16_t LANG_IDS[] PROGMEM = {0x0409, 0x0000};
static const char16_t* STR_LIST[] = {
    STR_MANUFACTURER
  , STR_PRODUCT
  , STR_SERIAL_NUMBER
};


struct descriptor_list_t* generate_descriptor_list(const UsbSetupPacket* req) {
  struct descriptor_list_t* head = 0;
  enum usb_descriptor_type_t type = req->wValue >> 8;
  uint8_t index = req->wValue & 0xFF;
  switch (type) {
    case DESC_TYPE_DEVICE:
      if (index == 0) {
        head = create_list_item(DESC_TYPE_DEVICE, &BODY_DEVICE, true);
      }
      break;
    case DESC_TYPE_STRING:
      if (index == 0) {
        head = create_list_item(DESC_TYPE_STRING, LANG_IDS, true);
      }
      else if (index-1 < sizeof(STR_LIST)/sizeof(char16_t)) {
        head = create_list_item(DESC_TYPE_STRING, STR_LIST[index-1], true);
      }
      break;
    default:
      break;
  }
  return head;
}

