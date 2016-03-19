#include "usb/descriptor.h"
#include <stdlib.h>
#include <avr/pgmspace.h>

typedef __CHAR16_TYPE__ char16_t;

// Return the number of 16b words in str.
// Codepoints that are split over two UTF16 char16_t values, add 2 to the returned length.
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

uint8_t usb_descriptor_size(
    const enum usb_descriptor_type_t type
  , const void* body
  , const bool body_in_flash
) {
  typedef struct usb_descriptor_header_t header;
  return usb_descriptor_body_size(type, body, body_in_flash) + sizeof(header);
}

uint8_t usb_descriptor_body_size(
    const enum usb_descriptor_type_t type
  , const void* body
  , const bool body_in_flash
) {
  typedef struct usb_descriptor_body_device_t body_device;
  typedef struct usb_descriptor_body_configuration_t body_configuration;
  typedef struct usb_descriptor_body_interface_t body_interface;
  typedef struct usb_descriptor_body_endpoint_t body_endpoint;

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

uint16_t get_list_total_length(const struct descriptor_list_t* head) {
  uint16_t total = 0;
  while (head && head->next != head) {
    total += head->header.bLength;
    head = head->next;
  }
  return total;
}

static void descriptor_list_append(
    struct descriptor_list_t* head
  , struct descriptor_list_t* item
) {
  while (head->next) {
    head = head->next;
  }
  head->next = item;
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

static const struct usb_descriptor_body_configuration_t BODY_CONFIG PROGMEM = {
    0 // to be filled in at runtime
  , 1
  , 1
  , 0
  , _BV(7) | _BV(6)
  , 25
};

static const struct usb_descriptor_body_interface_t BODY_INTERFACE PROGMEM = {
    0
  , 0
  , 0
  , 0xFF
  , 0
  , 0
  , 4
};

#define EP_NUM_MASK 0xF
#define EP_DIR_IN (0<<7)
#define EP_DIR_OUT (1<<7)


/*static const struct usb_descriptor_body_endpoint_t BODY_ENDPOINT_1 PROGMEM = {*/
/*    (1 & EP_NUM_MASK) | EP_DIR_OUT*/
/*  , 2*/
/*  , 256*/
/*  , 0*/
/*};*/

static const char16_t STR_MANUFACTURER[] PROGMEM = u"Ghent University";
static const char16_t STR_PRODUCT[] PROGMEM = u"IceTop event display";
static const char16_t STR_SERIAL_NUMBER[] PROGMEM = u"ICD-IT-001-0001";
static const char16_t STR_IFACE_DESCR[] PROGMEM = u"Steamshovel display";

#define LANG_ID_EN_US 0x0409
#define STRING_COUNT 4

static const uint16_t LANG_IDS[] PROGMEM = {LANG_ID_EN_US, 0x0000};
static const char16_t* const STR_EN_US[STRING_COUNT] = {
    STR_MANUFACTURER
  , STR_PRODUCT
  , STR_SERIAL_NUMBER
  , STR_IFACE_DESCR
};

static struct usb_descriptor_body_configuration_t descriptor_config;

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
      else if (index-1 < STRING_COUNT) {
        if (req->wIndex == LANG_ID_EN_US) {
          head = create_list_item(DESC_TYPE_STRING, STR_EN_US[index-1], true);
        }
      }
      break;
    case DESC_TYPE_CONFIGURATION:
      // Create appropriate configuration descriptor
      if (index == 0) {
        memcpy_P(&descriptor_config, &BODY_CONFIG, sizeof(BODY_CONFIG));
        head = create_list_item(DESC_TYPE_CONFIGURATION, &descriptor_config, false);
        descriptor_list_append(head, create_list_item(DESC_TYPE_INTERFACE, &BODY_INTERFACE, true));
/*        descriptor_list_append(head, create_list_item(DESC_TYPE_ENDPOINT, &BODY_ENDPOINT_1, true));*/
        // Calculate and fill in total configuration length
        descriptor_config.wTotalLength = get_list_total_length(head);
      }
      break;
    default:
      break;
  }
  return head;
}

