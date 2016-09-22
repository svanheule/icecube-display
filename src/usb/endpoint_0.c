#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "usb/endpoint_0.h"
#include "usb/device.h"
#include "usb/address.h"
#include "usb/endpoint.h"
#include "usb/configuration.h"
#include "usb/fifo.h"
#include "frame_queue.h"
#include "display_properties.h"

// Descriptor transaction definitions
#include "usb/descriptor.h"
#include <stdlib.h>
#include <string.h>

static inline uint8_t min(uint8_t a, uint8_t b) {
  return a < b ? a : b;
}

static void callback_set_address(struct control_transfer_t* transfer);
static void callback_set_configuration(struct control_transfer_t* transfer);

static void* init_data_in(struct control_transfer_t* transfer, size_t length) {
  transfer->data = malloc(length);
  if (transfer->data) {
    transfer->stage = CTRL_DATA_IN;
    transfer->data_length = length;
    transfer->data_done = 0;
  }
  return transfer->data;
}

void init_control_transfer(
      struct control_transfer_t *transfer
    , const struct usb_setup_packet_t *setup
) {
  transfer->callback_handshake = 0;
  transfer->callback_data = 0;
  transfer->stage = CTRL_SETUP;
  transfer->req = setup;
}

void cancel_control_transfer(struct control_transfer_t* transfer) {
  transfer->stage = CTRL_STALL;
  // Call any cancel callbacks that may perform clean-up
  if (transfer->callback_cancel) {
    transfer->callback_cancel();
  }
  // Release untransmitted data
  if (transfer->data) {
    free(transfer->data);
    transfer->data = 0;
  }
}

static inline void process_standard_request(struct control_transfer_t* transfer) {
  switch (transfer->req->bRequest) {
    case GET_STATUS:
      // default state fail; address state fail if EP!=0; config state fail if non-existant
      // Device: self-powered, configured for remote wake-up
      // Interface: zeros
      // Endpoint: halted (interrupt and bulk required, other optional)
      if (init_data_in(transfer, 2)) {
        switch (transfer->req->bmRequestType) {
          case (REQ_DIR_IN | REQ_TYPE_STANDARD | REQ_REC_DEVICE):
            *((uint16_t*) transfer->data) = DEVICE_STATUS_SELF_POWERED;
            break;
          case (REQ_DIR_IN | REQ_TYPE_STANDARD | REQ_REC_INTERFACE):
            *((uint16_t*) transfer->data) = 0;
            break;
          case (REQ_DIR_IN | REQ_TYPE_STANDARD | REQ_REC_ENDPOINT):
            // TODO Select the requested EP and return halted status
            // Currently no EP supports being halted, so always return 0
            *((uint16_t*) transfer->data) = 0;
            break;
          default:
            cancel_control_transfer(transfer);
            break;
        }
      }
      break;
    case SET_ADDRESS:
      // get address from SETUP, handshake, enable new address
      if (transfer->req->bmRequestType == (REQ_DIR_OUT | REQ_TYPE_STANDARD | REQ_REC_DEVICE)) {
        if (transfer->req->wIndex == 0 && transfer->req->wLength == 0) {
          // Set new address, but only enable *after* ZLP handshake
          transfer->stage = CTRL_HANDSHAKE_OUT;
          transfer->callback_handshake = callback_set_address;
        }
      }
      break;
    case GET_CONFIGURATION:
      if (transfer->req->bmRequestType == (REQ_DIR_IN | REQ_TYPE_STANDARD | REQ_REC_DEVICE)) {
        if(init_data_in(transfer, 1)) {
          *((uint8_t*) transfer->data) = get_configuration_index();
        }
      }
      break;
    case SET_CONFIGURATION:
      // if cfg=0 go to address state, otherwise go to configured state with selected config
      // if not addressed yet, consider this command invalid
      // reconfigure after handshake
      if (valid_configuration_index(transfer->req->wValue)) {
        transfer->stage = CTRL_HANDSHAKE_OUT;
        transfer->callback_handshake = callback_set_configuration;
      }
      break;
    case GET_DESCRIPTOR:
      // determine which descriptor to transmit and setup transaction
      if (transfer->req->bmRequestType == (REQ_DIR_IN | REQ_TYPE_STANDARD | REQ_REC_DEVICE)) {
        struct descriptor_list_t* head = generate_descriptor_list(transfer->req);
        struct descriptor_list_t* old_head;
        if (head) {
          // Send until requested size is reached OR all data is sent
          uint16_t list_length = get_list_total_length(head);
          uint16_t requested_length = transfer->req->wLength;
          uint16_t bytes_left = min(requested_length, list_length);

          if (init_data_in(transfer, bytes_left)) {
            const uint8_t HEADER_SIZE = sizeof(struct usb_descriptor_header_t);
            uint8_t body_length, len;
            uint8_t* write_ptr = (uint8_t*) transfer->data;
            while (head && bytes_left) {
              // Copy header
              len = min(bytes_left, HEADER_SIZE);
              memcpy(write_ptr, &head->header, len);
              write_ptr += len;
              bytes_left -= len;
              // Copy body
              body_length = head->header.bLength - HEADER_SIZE;
              len = min(bytes_left, body_length);
              memcpy_memspace(head->memspace, write_ptr, head->body, len);

              write_ptr += len;
              bytes_left -= len;

              // Proceed to next item and free current
              old_head = head;
              head = head->next;
              free(old_head);
            }
          }

          // Free any remaining descriptors
          while (head) {
            old_head = head;
            head = head->next;
            free(old_head);
          }
        }
      }
      break;
    default:
      break;
  }
}

static void callback_set_address(struct control_transfer_t* transfer) {
  // Enable new address
  const uint8_t address = (uint8_t) (transfer->req->wValue & 0x7F);
  usb_set_address(address);
  if (address) {
    set_device_state(ADDRESSED);
  }
}

static void callback_set_configuration(struct control_transfer_t* transfer) {
  set_configuration_index(transfer->req->wValue);
  if (transfer->req->wValue == 0) {
    set_device_state(DEFAULT);
  }
  else {
    set_device_state(CONFIGURED);
  }
}

#define VENDOR_REQUEST_PUSH_FRAME 1
static struct frame_buffer_t* usb_frame;
static uint8_t* usb_frame_buffer_ptr;
static void callback_data_usb_frame(struct control_transfer_t* transfer);
static void callback_cancel_usb_frame();

#define VENDOR_REQUEST_DISPLAY_PROPERTIES 2

static inline void process_vendor_request(struct control_transfer_t* transfer) {
  if (transfer->req->bmRequestType == (REQ_DIR_OUT | REQ_TYPE_VENDOR | REQ_REC_INTERFACE)) {
    // If correct request and request length
    if (   transfer->req->bRequest == VENDOR_REQUEST_PUSH_FRAME
        && transfer->req->wLength == get_display_buffer_size()
    ) {
      usb_frame = create_frame();
      if (usb_frame) {
        usb_frame->flags = FRAME_FREE_AFTER_DRAW;
        usb_frame_buffer_ptr = (uint8_t*) usb_frame->buffer;
        transfer->stage = CTRL_DATA_OUT;
        transfer->callback_data = callback_data_usb_frame;
        transfer->callback_cancel = callback_cancel_usb_frame;
      }
    }
  }
  else if (transfer->req->bmRequestType == (REQ_DIR_IN | REQ_TYPE_VENDOR | REQ_REC_INTERFACE)) {
    if (transfer->req->bRequest == VENDOR_REQUEST_DISPLAY_PROPERTIES) {
      // Get requested length or clip at length of available data
      const struct dp_tlv_item_t* props_head = get_display_properties_P();
      uint16_t display_properties_size = sizeof(uint16_t) + get_tlv_list_length_P(props_head);
      uint16_t remaining = transfer->req->wLength;
      if (!(transfer->req->wLength < display_properties_size)) {
        remaining = display_properties_size;
      }

      // Transmit data if at least 2 bytes are requested, otherwise stall
      if (remaining >= sizeof(display_properties_size)) {
        // Allocate buffer
        uint8_t* buffer = init_data_in(transfer, remaining);
        if (buffer) {
          // Copy properties header: size
          *((uint16_t*) buffer) = display_properties_size;
          buffer += sizeof(display_properties_size);
          remaining -= sizeof(display_properties_size);
          // Copy properties TLV fields (if there is any remaining space)
          struct dp_tlv_item_t item;
          memcpy_P(&item, props_head, sizeof(struct dp_tlv_item_t));
          while (remaining && item.type != DP_END) {
            if (remaining--) {
              *buffer++ = item.type;
            }
            if (remaining--) {
              *buffer++ = item.length;
            }
            if (remaining) {
              size_t copy_len = min(item.length, remaining);
              memcpy_memspace(item.memspace, (void*) buffer, item.data, copy_len);
              buffer += copy_len;
              remaining -= copy_len;
            }
            // Proceed to the next list item
            ++props_head;
            memcpy_P(&item, props_head, sizeof(struct dp_tlv_item_t));
          }
          // Finish by marking data as ready
          transfer->stage = CTRL_DATA_IN;
        }
      }
    }
  }
}

static void callback_data_usb_frame(struct control_transfer_t* transfer) {
  // Get frame data
  usb_frame_buffer_ptr += fifo_read(usb_frame_buffer_ptr, fifo_size());
  // Push if all data was received
  uint8_t* end_ptr = (uint8_t*) usb_frame->buffer + get_display_buffer_size();
  if (usb_frame_buffer_ptr == end_ptr) {
    if(!push_frame(usb_frame)) {
      // Prevent memory leaks and dangling pointers
      destroy_frame(usb_frame);
      usb_frame = 0;
    }
    transfer->stage = CTRL_HANDSHAKE_OUT;
  }
}

static void callback_cancel_usb_frame() {
  if (usb_frame) {
    destroy_frame(usb_frame);
    usb_frame = 0;
  }
}

void process_setup(struct control_transfer_t* transfer) {
  switch (GET_REQUEST_TYPE(transfer->req->bmRequestType)) {
    case REQ_TYPE_STANDARD:
      process_standard_request(transfer);
      break;
    case REQ_TYPE_VENDOR:
      process_vendor_request(transfer);
      break;
    default:
      break;
  }
}

