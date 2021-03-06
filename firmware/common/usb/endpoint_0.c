#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "usb/endpoint_0.h"
#include "usb/device.h"
#include "usb/address.h"
#include "usb/endpoint.h"
#include "usb/configuration.h"
#include "frame_queue.h"
#include "display_properties.h"
#include "frame_timer.h"

// Descriptor transaction definitions
#include "usb/descriptor.h"
#include <stdlib.h>
#include <string.h>

static inline uint16_t min(uint16_t a, uint16_t b) {
  return a < b ? a : b;
}

static void callback_set_address(struct control_transfer_t* transfer);
static void callback_set_configuration(struct control_transfer_t* transfer);

static void callback_default_data_in(struct control_transfer_t* transfer) {
  if (transfer->data_done == transfer->data_length) {
    free(transfer->data);
    transfer->data = 0;
    transfer->stage = CTRL_HANDSHAKE_IN;
  }
}

static void callback_default_cancel(struct control_transfer_t* transfer) {
  // Release untransmitted data
  if (transfer->data) {
    free(transfer->data);
    transfer->data = 0;
  }
}

static void* init_data_in(struct control_transfer_t* transfer, size_t length) {
  transfer->data = malloc(length);
  if (transfer->data) {
    transfer->stage = CTRL_DATA_IN;
    transfer->data_length = length;
    transfer->data_done = 0;
    transfer->callback_data = callback_default_data_in;
    transfer->callback_cancel = callback_default_cancel;
  }
  return transfer->data;
}

void init_control_transfer(
      struct control_transfer_t *transfer
    , const struct usb_setup_packet_t *setup
) {
  transfer->callback_handshake = 0;
  transfer->callback_data = 0;
  transfer->callback_cancel = 0;
  transfer->data = 0;
  transfer->data_length = 0;
  transfer->data_done = 0;
  transfer->stage = CTRL_SETUP;
  transfer->req = setup;
}

void cancel_control_transfer(struct control_transfer_t* transfer) {
  // Call any cancel callbacks that may perform clean-up
  if (transfer->callback_cancel) {
    transfer->callback_cancel(transfer);
  }
  transfer->stage = CTRL_STALL;
}

void control_mark_data_done(struct control_transfer_t* transfer, uint16_t length) {
  transfer->data_done += length;

  if (transfer->callback_data) {
    transfer->callback_data(transfer);
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
#if defined(DEVICE_SELF_POWERED) && DEVICE_SELF_POWERED
            *((uint16_t*) transfer->data) = DEVICE_STATUS_SELF_POWERED;
#else
            *((uint16_t*) transfer->data) = 0;
#endif
            break;
          case (REQ_DIR_IN | REQ_TYPE_STANDARD | REQ_REC_INTERFACE):
            *((uint16_t*) transfer->data) = 0;
            break;
          case (REQ_DIR_IN | REQ_TYPE_STANDARD | REQ_REC_ENDPOINT):
            *((uint16_t*) transfer->data) =
                  endpoint_is_stalled(transfer->req->wIndex & 0xf) ? 1 : 0;
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
          uint16_t bytes_left = min(transfer->req->wLength, get_list_total_length(head));
          uint8_t* buffer = init_data_in(transfer, bytes_left);

          if (buffer) {
            const uint8_t HEADER_SIZE = sizeof(struct usb_descriptor_header_t);
            uint8_t body_length;
            uint16_t len;
            while (head && bytes_left) {
              // Copy header
              len = min(bytes_left, HEADER_SIZE);
              memcpy(buffer, &head->header, len);
              buffer += len;
              bytes_left -= len;
              // Copy body
              body_length = head->header.bLength - HEADER_SIZE;
              len = min(bytes_left, body_length);
              memcpy_memspace(head->memspace, buffer, head->body, len);

              buffer += len;
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
    case SET_FEATURE:
      if (transfer->req->bmRequestType == (REQ_DIR_OUT | REQ_TYPE_STANDARD | REQ_REC_ENDPOINT)) {
        if (transfer->req->wValue == ENDPOINT_HALT) {
          uint8_t ep_num = transfer->req->wIndex & 0xf;
          if (endpoint_stall(ep_num)) {
            transfer->stage = CTRL_HANDSHAKE_OUT;
          }
        }
      }
      break;
    case CLEAR_FEATURE:
      if (transfer->req->bmRequestType == (REQ_DIR_OUT | REQ_TYPE_STANDARD | REQ_REC_ENDPOINT)) {
        if (transfer->req->wValue == ENDPOINT_HALT) {
          uint8_t ep_num = transfer->req->wIndex & 0xf;
          endpoint_reset_data_toggle(ep_num);
          // Reset endpoint internal (non USB standard) state
          const struct configuration_t* config = get_configuration_P(get_configuration_index());
          if (config && ep_num < pgm_read_byte(&config->endpoint_count)) {
            // Copy ep_config_t pointer
            struct ep_config_t* ep_config_ptr;
            memcpy_P(&ep_config_ptr, &config->ep_config_list, sizeof(ep_config_ptr));
            ep_config_ptr += ep_num;
            // Copy init function pointer
            void (*ep_init)();
            memcpy_P(&ep_init, &(ep_config_ptr->init), sizeof(ep_init));
            if (ep_init) {
              ep_init();
            }
            else {
              endpoint_init_default(ep_num);
            }
          }
          // Clear stall
          if (endpoint_clear_stall(ep_num)) {
            transfer->stage = CTRL_HANDSHAKE_OUT;
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
  const uint8_t address = (uint8_t) transfer->req->wValue;
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
    // TODO Reset frame receiving state machine
    // Step 1: Split off current logic from EP0
    // Step 2: move/copy logic to EP1
    set_device_state(CONFIGURED);
  }
}

// Remote frame transfer
static struct frame_buffer_t* usb_frame;
static uint8_t* usb_frame_buffer;
static uint16_t usb_frame_done;
static void callback_data_usb_frame(struct control_transfer_t* transfer);
static void callback_cancel_usb_frame();

// EEPROM read/write
#if defined(__MK20DX256__)
extern uint8_t __eeprom_start[];
#elif defined(__AVR_ARCH__) && __AVR_ARCH__ == 5
static uint8_t* const __eeprom_start = (uint8_t*) 0x810000;
#endif
const uint16_t EEPROM_SIZE = E2END + 1;
static void callback_data_eeprom_write(struct control_transfer_t* transfer);
static void callback_handshake_eeprom_write(struct control_transfer_t* transfer);

// Frame draw status/sync
#define FRAME_DRAW_STATUS_SIZE (sizeof(struct display_frame_usb_phase_t))

static inline void process_vendor_request(struct control_transfer_t* transfer) {
  if (transfer->req->bmRequestType == (REQ_DIR_OUT | REQ_TYPE_VENDOR | REQ_REC_DEVICE)) {
    if (transfer->req->bRequest == VENDOR_REQUEST_PUSH_FRAME) {
      const size_t buffer_size = get_frame_buffer_size();
      if (!usb_frame) {
        usb_frame = create_frame();
        if (usb_frame) {
          usb_frame->flags = FRAME_FREE_AFTER_DRAW;
          usb_frame_done = 0;
          usb_frame_buffer = usb_frame->buffer;
        }
      }
      if (usb_frame) {
        transfer->data = usb_frame_buffer;
        transfer->data_length = min(buffer_size - usb_frame_done, transfer->req->wLength);
        transfer->data_done = 0;
        transfer->callback_data = callback_data_usb_frame;
        transfer->callback_cancel = callback_cancel_usb_frame;
        transfer->stage = CTRL_DATA_OUT;
      }
    }
    else if (transfer->req->bRequest == VENDOR_REQUEST_EEPROM_WRITE
          && transfer->req->wIndex + transfer->req->wLength <= EEPROM_SIZE)
    {
      transfer->data = malloc(transfer->req->wLength);
      if (transfer->data) {
        transfer->data_length = transfer->req->wLength;
        transfer->data_done = 0;
        transfer->callback_data = callback_data_eeprom_write;
        transfer->callback_handshake = callback_handshake_eeprom_write;
        transfer->callback_cancel = callback_default_cancel;
        transfer->stage = CTRL_DATA_OUT;
      }
    }
    else if (transfer->req->bRequest == VENDOR_REQUEST_FRAME_DRAW_SYNC) {
      // wValue is the number of ms the frame timer should be corrected.
      // This can be factored as (40*frame_correction + ms_correction) to split the
      // display counter value correction and display phase correction.
      int16_t full_correction = (int16_t) transfer->req->wValue;
      // Round the frame number correction such that the ms_correction will have the smallest
      // possible absolute value.
      int16_t frame_correction = (full_correction + MS_PER_FRAME/2) / MS_PER_FRAME;
      int8_t ms_correction = full_correction - frame_correction*MS_PER_FRAME;

      correct_display_frame_phase(ms_correction);
      if (correct_display_frame_counter(frame_correction)) {
        transfer->stage = CTRL_HANDSHAKE_OUT;
      }
      else {
        transfer->stage = CTRL_STALL;
      }
    }
  }
  else if (transfer->req->bmRequestType == (REQ_DIR_IN | REQ_TYPE_VENDOR | REQ_REC_DEVICE)) {
    if (transfer->req->bRequest == VENDOR_REQUEST_DISPLAY_PROPERTIES) {
      // Get requested length or clip at length of available data
      const struct dp_tlv_item_t* props_head = get_display_properties_P();
      uint16_t display_properties_size = sizeof(uint16_t) + get_tlv_list_length_P(props_head);
      uint16_t remaining = min(transfer->req->wLength, display_properties_size);

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
          while (remaining && item.type != TLV_TYPE_END) {
            if (remaining--) {
              *buffer++ = item.type;
              if (remaining--) {
                *buffer++ = item.length;
                if (remaining) {
                  uint16_t copy_len = min(item.length, remaining);
                  memcpy_memspace(item.memspace, (void*) buffer, item.data, copy_len);
                  buffer += copy_len;
                  remaining -= copy_len;
                }
              }
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
    else if (transfer->req->bRequest == VENDOR_REQUEST_EEPROM_READ) {
      uint16_t length = min(EEPROM_SIZE, transfer->req->wLength);
      uint16_t offset = transfer->req->wIndex;
      if (offset <= EEPROM_SIZE && length + offset <= EEPROM_SIZE) {
        uint8_t* buffer = init_data_in(transfer, length);
        if (buffer) {
          eeprom_read_block(buffer, (uint8_t*) __eeprom_start + offset, length);
        }
      }
    }
    else if (transfer->req->bRequest == VENDOR_REQUEST_FRAME_DRAW_STATUS) {
      if (transfer->req->wLength == FRAME_DRAW_STATUS_SIZE
          && init_data_in(transfer, FRAME_DRAW_STATUS_SIZE)) {
        struct display_frame_usb_phase_t* buffer =
              (struct display_frame_usb_phase_t*) transfer->data;
        if (!get_display_frame_usb_phase(buffer)) {
          buffer->usb_frame_counter = 0xffff;
        }
      }
    }
  }
}

static void callback_data_usb_frame(struct control_transfer_t* transfer) {
  // Push if all data was received
  if (transfer->data_length == transfer->data_done) {
    usb_frame_done += transfer->data_length;
    usb_frame_buffer += transfer->data_length;
    if (usb_frame_done == get_frame_buffer_size()) {
      if(!push_frame(usb_frame)) {
        // Prevent memory leaks and dangling pointers
        destroy_frame(usb_frame);
      }
      usb_frame = 0;
      usb_frame_buffer = 0;
      usb_frame_done = 0;
    }
    transfer->stage = CTRL_HANDSHAKE_OUT;
  }
}

static void callback_cancel_usb_frame() {
  if (usb_frame) {
    destroy_frame(usb_frame);
    usb_frame = 0;
    usb_frame_buffer = 0;
    usb_frame_done = 0;
  }
}

static void callback_data_eeprom_write(struct control_transfer_t *transfer) {
  if (transfer->data_done == transfer->data_length) {
    transfer->stage = CTRL_HANDSHAKE_OUT;
  }
}

static void callback_handshake_eeprom_write(struct control_transfer_t *transfer) {
  if (transfer->data) {
    uint8_t* dest = (uint8_t*) __eeprom_start + transfer->req->wIndex;
    eeprom_update_block(transfer->data, dest, transfer->data_length);
    free(transfer->data);
    transfer->data = 0;
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

