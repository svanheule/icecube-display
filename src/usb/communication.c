/** \defgroup usb_endpoint_control Default control endpoint
  * \ingroup usb_endpoint
  * \brief Default control endpoint (endpoint 0) behaviour.
  * \details Control transfers are handled entirely asynchronously via interrupts. When the device
  *   has to wait for an answer from the host for example, this means that the firmware won't wait
  *   and poll to see if an answer has been received, but resumes normal operation until an
  *   interrupt is fired. This implies the use of task specific callbacks, since different setup
  *   request require different actions to be performed, possibly at different points in the
  *   request's lifetime.
  *
  *   The control endpoint process requests in the following way, as illustrated by the diagram
  *   below:
  *   - Initially, the control endpoint's FSM is in the ::CTRL_IDLE state and transitions to
  *     the ::CTRL_SETUP state if a new setup packet has been received.
  *   - If the direction is IN, the endpoint prepares the data for transmission and enters the
  *     ::CTRL_DATA_IN state. Once this data is transmitted, the FSM proceeds to the
  *     ::CTRL_HANDSHAKE_IN state, waiting for a zero-length packet (ZLP) confirming the data was
  *     correctly received. This is then followed by possible a post-handshake action.
  *     and state if needed. The FSM then transitions back to the ::CTRL_IDLE state, waiting
  *     for a new setup request.
  *   - If the direction of the request is OUT, it is possible no extra data except for the request
  *     will be provided by the host. In this case, the FSM transitions to ::CTRL_HANDSHAKE_OUT to
  *     acknowledge the transaction. If extra data will be sent, the FSM first enters the
  *     ::CTRL_DATA_OUT state, waiting to receive more data.
  *   - If an unknown or bad request is received, or the device is unable
  *     handle the requested/provided data, the FSM will enter the ::CTRL_STALL state and
  *     stall the endpoint, notifying the host of a failed transaction.
  *     The stall is automatically cleared once the next setup request is received.
  *
  *   \dot
  *     digraph setup_fsm {
  *       node [shape=record];
  *       idle [shape=doublecircle, label=IDLE, URL="\ref ::CTRL_IDLE"];
  *       stall [style=filled, fillcolor=orange, label="STALL", URL="\ref ::CTRL_STALL"];
  *       setup [label=SETUP, URL="\ref ::CTRL_SETUP"];
  *       data_in [label="{DATA_IN | Send IN frame(s) with data}", URL="\ref ::CTRL_DATA_IN"];
  *       data_out [
  *           label="{DATA_OUT | Receive OUT frame(s) with data}"
  *         , URL="\ref ::CTRL_DATA_OUT"
  *       ];
  *       handshake_in [
  *           label="{HANDSHAKE_IN | Wait for ZLP OUT frame}"
  *         , URL="\ref ::CTRL_HANDSHAKE_IN"
  *       ];
  *       handshake_out [
  *           label="{HANDSHAKE_OUT | Send ZLP IN frame}"
  *         , URL="\ref ::CTRL_HANDSHAKE_OUT"
  *       ];
  *       post_handshake [
  *           label="{POST_HANDSHAKE | Post-ZLP action}"
  *          , URL="\ref ::CTRL_POST_HANDSHAKE"
  *       ];
  *       {rank=same stall idle} -> setup;
  *       subgraph data_transmission {
  *         setup -> {data_in data_out handshake_out};
  *         data_in -> handshake_in -> post_handshake;
  *         data_out -> handshake_out -> post_handshake;
  *       }
  *       {handshake_in handshake_out post_handshake} -> idle;
  *     }
  *   \enddot
  */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "usb/std.h"
#include "usb/device.h"
#include "usb/endpoint.h"
#include "usb/configuration.h"
#include "usb/fifo.h"
#include "usb/led.h"
#include "frame_queue.h"
#include "display_properties.h"

// Descriptor transaction definitions
#include "usb/descriptor.h"
#include <stdlib.h>
#include <string.h>

/// \brief Constants used to indicate the default control endpoint's current state.
/// \ingroup usb_endpoint_control
enum control_stage_t {
    CTRL_IDLE ///< Control endpoint idle
  , CTRL_SETUP ///< New setup request received
  , CTRL_STALL ///< Control endpoint stalled
  , CTRL_DATA_IN ///< Sending data to host
  , CTRL_DATA_OUT ///< Receiving data from host
  , CTRL_HANDSHAKE_IN ///< Handshaking IN transfer
  , CTRL_HANDSHAKE_OUT ///< Handshaking OUT transfer
  , CTRL_POST_HANDSHAKE ///< Performing post-handshake action
};

/// \brief Control transfer state tracking.
/// \ingroup usb_endpoint_control
struct control_transfer_t {
  /// Stage the request is currently in.
  enum control_stage_t stage;
  /// Setup request that is currently being handled.
  const struct usb_setup_packet_t* req;
  /// Data to be transmitted to the host.
  void* data_in;
  /// Total length of \a data_in.
  uint16_t data_in_length;
  /// Transmitted length of \a data_in.
  uint16_t data_in_done;
  /// Function to be called when new data is received.
  void (*callback_data_out)(struct control_transfer_t* transfer);
  /// Function to be called *after* the ZLP handshake is sent/received.
  void (*callback_handshake)(struct control_transfer_t* transfer);
  /// Cleanup function to be called when the transfer is cancelled.
  void (*callback_cancel)();
};

static void callback_set_address(struct control_transfer_t* transfer);
static void callback_set_configuration(struct control_transfer_t* transfer);

static void* init_data_in(struct control_transfer_t* transfer, size_t length) {
  transfer->data_in = malloc(length);
  if (transfer->data_in) {
    transfer->stage = CTRL_DATA_IN;
    transfer->data_in_length = length;
    transfer->data_in_done = 0;
  }
  return transfer->data_in;
}

static void cancel_control_transfer(struct control_transfer_t* transfer) {
  transfer->stage = CTRL_STALL;
  // Call any cancel callbacks that may perform clean-up
  if (transfer->callback_cancel) {
    transfer->callback_cancel();
  }
  // Release untransmitted data
  if (transfer->data_in) {
    free(transfer->data_in);
    transfer->data_in = 0;
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
            *((uint16_t*) transfer->data_in) = DEVICE_STATUS_SELF_POWERED;
            break;
          case (REQ_DIR_IN | REQ_TYPE_STANDARD | REQ_REC_INTERFACE):
            *((uint16_t*) transfer->data_in) = 0;
            break;
          case (REQ_DIR_IN | REQ_TYPE_STANDARD | REQ_REC_ENDPOINT):
            // TODO Select the requested EP and return halted status
            // Currently no EP supports being halted, so always return 0
            *((uint16_t*) transfer->data_in) = 0;
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
          UDADDR = (uint8_t) (transfer->req->wValue & 0x7F);
          transfer->stage = CTRL_HANDSHAKE_OUT;
          transfer->callback_handshake = callback_set_address;
        }
      }
      break;
    case GET_CONFIGURATION:
      if (transfer->req->bmRequestType == (REQ_DIR_IN | REQ_TYPE_STANDARD | REQ_REC_DEVICE)) {
        if(init_data_in(transfer, 1)) {
          *((uint8_t*) transfer->data_in) = get_configuration_index();
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
          uint16_t bytes_left = requested_length < list_length ? requested_length : list_length;

          if (init_data_in(transfer, bytes_left)) {
            const uint8_t HEADER_SIZE = sizeof(struct usb_descriptor_header_t);
            uint8_t body_length, len;
            uint8_t* write_ptr = (uint8_t*) transfer->data_in;
            while (head && bytes_left) {
              // Copy header
              len = bytes_left < HEADER_SIZE ? bytes_left : HEADER_SIZE;
              memcpy(write_ptr, &head->header, len);
              write_ptr += len;
              bytes_left -= len;
              // Copy body
              body_length = head->header.bLength - HEADER_SIZE;
              len = bytes_left < body_length ? bytes_left : body_length;
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
  UDADDR |= _BV(ADDEN);
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
        transfer->callback_data_out = callback_data_usb_frame;
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
              size_t copy_len = item.length < remaining ? item.length : remaining;
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

static inline void process_setup(struct control_transfer_t* transfer) {
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

// Interrupt handling
#define CLEAR_UEINTX(flag) UEINTX &= ~(1<<flag)

static inline void clear_setup() {
  CLEAR_UEINTX(RXSTPI);
}
static inline void clear_in() {
  CLEAR_UEINTX(TXINI);
}
static inline void clear_out() {
  CLEAR_UEINTX(RXOUTI);
}

#define FLAG_IS_SET(reg, flag) (reg & (1<<flag))
#define CLEAR_FLAG(reg, flag) reg &= ~(1<<flag)
#define SET_FLAG(reg, flag) reg |= (1<<flag)

ISR(USB_COM_vect) {
  trip_led();

  // Process USB transfers
  if (FLAG_IS_SET(UEINT, 0)) {
    endpoint_push(0);
    static struct control_transfer_t control_transfer;
    static struct usb_setup_packet_t setup_packet;

    if (FLAG_IS_SET(UEIENX, RXSTPE) && FLAG_IS_SET(UEINTX, RXSTPI)) {
      CLEAR_FLAG(UEIENX, TXINE);
      CLEAR_FLAG(UEIENX, RXOUTE);

      if (control_transfer.stage != CTRL_IDLE && control_transfer.stage != CTRL_STALL) {
        cancel_control_transfer(&control_transfer);
      }
      control_transfer.callback_handshake = 0;
      control_transfer.callback_data_out = 0;
      control_transfer.callback_cancel = 0;
      control_transfer.stage = CTRL_SETUP;

      size_t read = fifo_read(&setup_packet, sizeof(struct usb_setup_packet_t));
      if (read == sizeof(struct usb_setup_packet_t)) {
        control_transfer.req = &setup_packet;
        process_setup(&control_transfer);
      }

      if (
           (control_transfer.stage == CTRL_DATA_IN)
        || (control_transfer.stage == CTRL_HANDSHAKE_OUT)
      ) {
        SET_FLAG(UEIENX, TXINE);
      }
      else if (
           (control_transfer.stage == CTRL_DATA_OUT)
        || (control_transfer.stage == CTRL_HANDSHAKE_IN)
      ) {
        SET_FLAG(UEIENX, RXOUTE);
      }
      else {
        SET_FLAG(UECONX, STALLRQ);
      }

      clear_setup();
    }

    if (FLAG_IS_SET(UEIENX, TXINE) && FLAG_IS_SET(UEINTX, TXINI)) {
      if (control_transfer.stage == CTRL_DATA_IN) {
        // Send remaining transaction data
        uint16_t fifo_free = fifo_size() - fifo_byte_count();
        uint16_t transfer_left = control_transfer.data_in_length - control_transfer.data_in_done;
        uint16_t length = transfer_left < fifo_free ? transfer_left : fifo_free;
        uint8_t* data = (uint8_t*) control_transfer.data_in + control_transfer.data_in_done;
        control_transfer.data_in_done += fifo_write(data, length);
        clear_in();
        if (control_transfer.data_in_done == control_transfer.data_in_length) {
          free(control_transfer.data_in);
          control_transfer.data_in = 0;
          control_transfer.stage = CTRL_HANDSHAKE_IN;
          CLEAR_FLAG(UEIENX, TXINE);
        }
      }
      else if (control_transfer.stage == CTRL_HANDSHAKE_OUT) {
        // Send ZLP handshake
        clear_in();
        if (control_transfer.callback_handshake) {
          control_transfer.stage = CTRL_POST_HANDSHAKE;
        }
        else {
          control_transfer.stage = CTRL_IDLE;
          CLEAR_FLAG(UEIENX, TXINE);
        }
      }
      else if (control_transfer.stage == CTRL_POST_HANDSHAKE) {
        control_transfer.callback_handshake(&control_transfer);
        control_transfer.stage = CTRL_IDLE;
        CLEAR_FLAG(UEIENX, TXINE);
      }
      else {
        CLEAR_FLAG(UEIENX, TXINE);
      }
    }

    if (FLAG_IS_SET(UEIENX, RXOUTE) && FLAG_IS_SET(UEINTX, RXOUTI)) {
      if (control_transfer.stage == CTRL_DATA_OUT) {
        // Process incoming data
        if (control_transfer.callback_data_out) {
          control_transfer.callback_data_out(&control_transfer);
        }
        clear_out();
        if (control_transfer.stage == CTRL_HANDSHAKE_OUT) {
          SET_FLAG(UEIENX, TXINE);
          CLEAR_FLAG(UEIENX, RXOUTE);
        }
      }
      else if (control_transfer.stage == CTRL_HANDSHAKE_IN) {
        // Acknowledge ZLP handshake
        clear_out();
        // Since acknowledging the handshake doesn't require waiting for the host, perform
        // any callback immediately
        if (control_transfer.callback_handshake) {
          control_transfer.callback_handshake(&control_transfer);
        }
        control_transfer.stage = CTRL_IDLE;
        CLEAR_FLAG(UEIENX, RXOUTE);
      }
      else {
        // Ignore data
        clear_out();
        CLEAR_FLAG(UEIENX, RXOUTE);
      }
    }

    // Restore endpoint number
    endpoint_pop();
  }

}
