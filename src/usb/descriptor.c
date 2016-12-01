#include "usb/descriptor.h"
#include "usb/endpoint.h"
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

typedef __CHAR16_TYPE__ char16_t;

// Return the number of 16b words in str.
// Codepoints that are split over two UTF16 char16_t values add 2 to the returned length.
static size_t strlen16(const char16_t* str) {
  const char16_t* end = str;
  while (*end != 0) {
    ++end;
  }
  return end-str;
}

static size_t strlen16_P(const char16_t* str) {
  const char16_t* end = str;
  uint16_t word = pgm_read_word((const uint16_t*) end);
  // See strlen16_E for more information on `word != 0xffff`
  while (word != 0 && word != 0xffff) {
    ++end;
    word = pgm_read_word((const uint16_t*) end);
  }
  return end-str;
}

/* It may occur that the EEPROM was left unprogrammed, so all reads will return 0xFFFF.
 * In UTF-16, the word 0xFFFF however is not valid. String length determination therefore stops
 * either at a 0x0000 (valid termination) or at 0xFFFF (invalid termination).
 */
static size_t strlen16_E(const char16_t* str) {
  const char16_t* end = str;
  uint16_t word = eeprom_read_word((const uint16_t*) end);
  while (word != 0 && word != 0xffff) {
    ++end;
    word = eeprom_read_word((const uint16_t*) end);
  }
  return end-str;
}

uint8_t usb_descriptor_size(
    const enum usb_descriptor_type_t type
  , const void* body
  , const enum memspace_t memspace
) {
  typedef struct usb_descriptor_header_t header;
  return usb_descriptor_body_size(type, body, memspace) + sizeof(header);
}

uint8_t usb_descriptor_body_size(
    const enum usb_descriptor_type_t type
  , const void* body
  , const enum memspace_t memspace
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
      switch (memspace) {
        case MEMSPACE_RAM:
          return 2*strlen16((const char16_t*) body);
          break;
        case MEMSPACE_PROGMEM:
          return 2*strlen16_P((const char16_t*) body);
          break;
        case MEMSPACE_EEPROM:
          return 2*strlen16_E((const char16_t*) body);
          break;
        default:
          return 0;
          break;
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
  , const enum memspace_t memspace
) {
  typedef struct descriptor_list_t item_t;
  item_t* item = (item_t*) malloc(sizeof(item_t));
  if (item) {
    item->header.bLength = usb_descriptor_size(type, body, memspace);
    item->header.bDescriptorType = type;
    item->memspace = memspace;
    item->body = body;
    item->next = 0;
  }
  return item;
}

uint16_t get_list_total_length(const struct descriptor_list_t* head) {
  uint16_t total = 0;
  while (head) {
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
  , USB_CONFIG_ATTRIBUTES(1, 0)
  , 25
};

static const struct usb_descriptor_body_interface_t BODY_INTERFACE PROGMEM = {
    0
  , 0
  , 1
  , 0xFF
  , 0
  , 0
  , 4
};

static const struct usb_descriptor_body_endpoint_t BULK_ENDPOINT PROGMEM = {
    1
  , EP_TYPE_BULK
  , 64
  , 0
};

static const char16_t STR_MANUFACTURER[] PROGMEM = u"Ghent University";
static const char16_t STR_PRODUCT[] PROGMEM = u"IceTop event display";
extern const char16_t STR_SERIAL_NUMBER[] EEMEM;
static const char16_t STR_IFACE_DESCR[] PROGMEM = u"Steamshovel display";

#define LANG_ID_EN_US 0x0409
#define STRING_COUNT 4

struct string_pointer_t {
  const char16_t* const p;
  const enum memspace_t memspace;
};

static const uint16_t LANG_IDS[] PROGMEM = {LANG_ID_EN_US, 0x0000};
static const struct string_pointer_t STR_EN_US[STRING_COUNT] PROGMEM = {
    {STR_MANUFACTURER, MEMSPACE_PROGMEM}
  , {STR_PRODUCT, MEMSPACE_PROGMEM}
  , {STR_SERIAL_NUMBER, MEMSPACE_EEPROM}
  , {STR_IFACE_DESCR, MEMSPACE_PROGMEM}
};

static struct usb_descriptor_body_configuration_t descriptor_config;

struct descriptor_list_t* generate_descriptor_list(const struct usb_setup_packet_t* req) {
  struct descriptor_list_t* head = 0;
  enum usb_descriptor_type_t type = req->wValue >> 8;
  uint8_t index = req->wValue & 0xFF;
  switch (type) {
    case DESC_TYPE_DEVICE:
      if (index == 0) {
        head = create_list_item(DESC_TYPE_DEVICE, &BODY_DEVICE, MEMSPACE_PROGMEM);
      }
      break;
    case DESC_TYPE_STRING:
      if (index == 0) {
        head = create_list_item(DESC_TYPE_STRING, LANG_IDS, MEMSPACE_PROGMEM);
      }
      else if (index-1 < STRING_COUNT) {
        if (req->wIndex == LANG_ID_EN_US) {
          const struct string_pointer_t* str = &STR_EN_US[index-1];
          head = create_list_item(
                DESC_TYPE_STRING
              , (const void*) pgm_read_word(&str->p)
              , pgm_read_byte(&str->memspace)
          );
        }
      }
      break;
    case DESC_TYPE_CONFIGURATION:
      // Create appropriate configuration descriptor
      if (index == 0) {
        memcpy_P(&descriptor_config, &BODY_CONFIG, sizeof(BODY_CONFIG));
        head = create_list_item(DESC_TYPE_CONFIGURATION, &descriptor_config, MEMSPACE_RAM);
        descriptor_list_append(
              head
            , create_list_item(DESC_TYPE_INTERFACE, &BODY_INTERFACE, MEMSPACE_PROGMEM)
        );
        descriptor_list_append(
              head
            , create_list_item(DESC_TYPE_ENDPOINT, &BULK_ENDPOINT, MEMSPACE_PROGMEM)
        );
        // Calculate and fill in total configuration length
        descriptor_config.wTotalLength = get_list_total_length(head);
      }
      break;
    default:
      break;
  }
  return head;
}
