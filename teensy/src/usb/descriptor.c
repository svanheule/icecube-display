#include "usb/descriptor.h"
#include <stdlib.h>
#include <string.h>
#include <avr/eeprom.h>

#include "kinetis/io.h"

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
        case MEMSPACE_PROGMEM:
          return 2*strlen16((const char16_t*) body);
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
static const struct usb_descriptor_body_device_t BODY_DEVICE = {
    .bcdUSB = 0x0200
  , .bDeviceClass = 0
  , .bDeviceSubClass = 0
  , .bDeviceProtocol = 0
  , .bMaxPacketSize = 64
  , .idVendor = 0x1ce3
  , .idProduct = 0x0002
  , .bcdDevice = 0x0320 // Use Teensy HW rev number: 3.2
  , .iManufacturer = 1
  , .iProduct = 2
  , .iSerialNumber = 4
  , .bNumConfigurations = 1
};

static const struct usb_descriptor_body_configuration_t BODY_CONFIG = {
    .wTotalLength = 0 // to be filled in at runtime
  , .bNumInterfaces = 1
  , .bConfigurationValue = 1
  , .iConfiguration = 0
  , .bmAttributes = _BV(7)
  , .bMaxPower = 1
};

static const struct usb_descriptor_body_interface_t BODY_INTERFACE = {
    .bInterfaceNumber = 0
  , .bAlternateSetting = 0
  , .bNumEndPoints = 2
  , .bInterfaceClass = 0xFF
  , .bInterfaceSubClass = 0
  , .bInterfaceProtocol = 0
  , .iInterface = 3
};

static const struct usb_descriptor_body_endpoint_t BULK_ENDPOINT = {
    .bEndpointAddress = 1
  , .bmAttributes = 2
  , .wMaxPacketSize = 64
  , .bInterval = 0
};

static const struct usb_descriptor_body_endpoint_t INTERRUPT_ENDPOINT = {
    .bEndpointAddress = 2 | _BV(7)
  , .bmAttributes = 3
  , .wMaxPacketSize = 4
  , .bInterval = 40
};

static const char16_t STR_MANUFACTURER[] = u"Ghent University";
static const char16_t STR_PRODUCT[] = u"IceCube event display";
static const char16_t STR_IFACE_DESCR[] = u"Steamshovel display";
extern const char16_t STR_SERIAL_NUMBER[];

#define LANG_ID_EN_US 0x0409

struct string_pointer_t {
  const char16_t* const p;
  const enum memspace_t memspace;
};

static const uint16_t LANG_IDS[] = {LANG_ID_EN_US, 0x0000};
static const struct string_pointer_t STR_EN_US[] = {
    {STR_MANUFACTURER, MEMSPACE_PROGMEM}
  , {STR_PRODUCT, MEMSPACE_PROGMEM}
  , {STR_IFACE_DESCR, MEMSPACE_PROGMEM}
  , {STR_SERIAL_NUMBER, MEMSPACE_EEPROM}
};
static const uint8_t STRING_COUNT = sizeof(STR_EN_US)/sizeof(struct string_pointer_t);

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
          head = create_list_item(DESC_TYPE_STRING, str->p, str->memspace);
        }
      }
      break;
    case DESC_TYPE_CONFIGURATION:
      // Create appropriate configuration descriptor
      if (index == 0) {
        memcpy(&descriptor_config, &BODY_CONFIG, sizeof(BODY_CONFIG));
        head = create_list_item(DESC_TYPE_CONFIGURATION, &descriptor_config, MEMSPACE_RAM);
        descriptor_list_append(
              head
            , create_list_item(DESC_TYPE_INTERFACE, &BODY_INTERFACE, MEMSPACE_PROGMEM)
        );
        descriptor_list_append(
              head
            , create_list_item(DESC_TYPE_ENDPOINT, &BULK_ENDPOINT, MEMSPACE_PROGMEM)
        );
        descriptor_list_append(
              head
            , create_list_item(DESC_TYPE_ENDPOINT, &INTERRUPT_ENDPOINT, MEMSPACE_PROGMEM)
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
