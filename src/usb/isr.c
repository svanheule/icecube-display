#include <avr/io.h>
#include <avr/interrupt.h>

#include "usb/std.h"
#include "usb/device.h"
#include "usb/endpoint.h"
#include "usb/configuration.h"
#include "usb/fifo.h"
#include "usb/led.h"
#include "frame_buffer.h"

// Descriptor transaction definitions
#include "usb/descriptor.h"
#include <stdlib.h>


#define FLAG_IS_SET(reg, flag) (reg & (1<<flag))
#define CLEAR_INTERRUPT(reg, flag) reg &= ~(1<<flag)
#define CLEAR_UDINT(flag) UDINT &= ~(1<<flag)
#define CLEAR_FLAG(reg, flag) reg &= ~(1<<flag)
#define SET_FLAG(reg, flag) reg |= (1<<flag)


ISR(USB_GEN_vect) {
  // VBUS transitions
  if (FLAG_IS_SET(USBINT, VBUSTI) && FLAG_IS_SET(USBCON, VBUSTE)) {
    CLEAR_INTERRUPT(USBINT, VBUSTI);
    if (FLAG_IS_SET(USBSTA, VBUS)) {
      // TODO reset all endpoints
      enable_pll();
      CLEAR_FLAG(USBCON, FRZCLK);
      SET_FLAG(UDIEN, EORSTE);
      SET_FLAG(UDIEN, SUSPI);
      set_device_state(POWERED);
    }
    else {
      CLEAR_FLAG(UDIEN, SUSPI);
      CLEAR_FLAG(UDIEN, WAKEUPE);
      CLEAR_FLAG(UDIEN, EORSTE);
      SET_FLAG(USBCON, FRZCLK);
      disable_pll();
      set_device_state(ATTACHED);
    }
  }

  // End-of-reset
  if (FLAG_IS_SET(UDINT, EORSTI) && FLAG_IS_SET(UDIEN, EORSTE)) {
    CLEAR_UDINT(EORSTI);
    // After reset, wait for activity
    CLEAR_UDINT(WAKEUPI);
    CLEAR_FLAG(UDIEN, WAKEUPE);
    SET_FLAG(UDIEN, SUSPE);

    // Load default configuration
    if (set_configuration_index(0)) {
      set_device_state(DEFAULT);
    }
  }

  // Wake-up, suspend
  if (FLAG_IS_SET(UDINT, SUSPI) && FLAG_IS_SET(UDIEN, SUSPE)) {
    CLEAR_FLAG(UDIEN, SUSPE);
    CLEAR_UDINT(WAKEUPI);
    SET_FLAG(UDIEN, WAKEUPE);

    SET_FLAG(USBCON, FRZCLK);
    disable_pll();
    set_device_state(SUSPENDED);
  }
  
  if (FLAG_IS_SET(UDINT, WAKEUPI) && FLAG_IS_SET(UDIEN, WAKEUPE)) {
    // Wake up device
    enable_pll();
    CLEAR_FLAG(USBCON, FRZCLK);

    // Clear WAKEUP, enable SUSPEND
    CLEAR_UDINT(SUSPI);
    CLEAR_UDINT(WAKEUPI);
    CLEAR_FLAG(UDIEN, WAKEUPE);
    SET_FLAG(UDIEN, SUSPE);

    // Track device state
    if (get_configuration_index() > 0) {
      set_device_state(CONFIGURED);
    }
    else if (FLAG_IS_SET(UDADDR, ADDEN)) {
      set_device_state(ADDRESSED);
    }
    else if (get_configuration_index() == 0) {
      set_device_state(DEFAULT);
    }
    else {
      set_device_state(POWERED);
    }
  }
}

static inline void clear_setup() {
  CLEAR_INTERRUPT(UEINTX, RXSTPI);
}
static inline void clear_in() {
  CLEAR_INTERRUPT(UEINTX, TXINI);
}
static inline void clear_out() {
  CLEAR_INTERRUPT(UEINTX, RXOUTI);
}


static void process_standard_request(const struct UsbSetupPacket_t* req) {
  bool valid_request = false;
  switch (req->bRequest) {
    case GET_STATUS:
      // default state fail; address state fail if EP!=0; config state fail if non-existant
      // Device: self-powered, configured for remote wake-up
      // Interface: zeros
      // Endpoint: halted (interrupt and bulk required, other optional)
      switch (req->bmRequestType) {
        case (REQ_DIR_IN | REQ_TYPE_STANDARD | REQ_REC_DEVICE):
          clear_setup();
          fifo_write_byte(_BV(DEVICE_STATUS_SELF_POWERED));
          fifo_write_byte(0);
          valid_request = true;
          break;
        case (REQ_DIR_IN | REQ_TYPE_STANDARD | REQ_REC_INTERFACE):
          clear_setup();
          fifo_write_byte(0);
          fifo_write_byte(0);
          valid_request = true;
          break;
        case (REQ_DIR_IN | REQ_TYPE_STANDARD | REQ_REC_ENDPOINT):
          clear_setup();
          // TODO Select the requested EP and return halted status
          // Currently no EP supports being halted, so always return 0
          fifo_write_byte(0);
          fifo_write_byte(0);
          valid_request = true;
          break;
        default:
          break;
      }
      if (valid_request) {
        clear_in();
        while (!(UEINTX & _BV(RXOUTI))) {}
        clear_out();
      }
      break;
    case SET_ADDRESS:
      // get address from SETUP, handshake, enable new address
      if (req->bmRequestType == (REQ_DIR_OUT | REQ_TYPE_STANDARD | REQ_REC_DEVICE)) {
        if (req->wIndex == 0 && req->wLength == 0) {
          clear_setup();
          // Set address
          uint8_t address = (uint8_t) (req->wValue & 0x7F);
          UDADDR = address;
          while (!(UEINTX & _BV(TXINI))) {}
          clear_in();
          // Wait until ZLP has been sent
          while (!(UEINTX & (_BV(TXINI)))) {}
          UDADDR |= _BV(ADDEN);
          if (address) {
            set_device_state(ADDRESSED);
          }
        }
      }
      break;
    case GET_CONFIGURATION:
      if (req->bmRequestType == (REQ_DIR_IN| REQ_TYPE_STANDARD | REQ_REC_DEVICE)) {
        // Acknowledge SETUP
        clear_setup();
        // Push current configuration number into FIFO and mark ready
        fifo_write_byte(get_configuration_index());
        clear_in();
        // Enable OUT interrupt to handshake transaction
        while (!(UEINTX & _BV(RXOUTI))) {}
        clear_out();
      }
      break;
    case SET_CONFIGURATION:
      // if cfg=0 go to address state, otherwise go to configured state with selected config
      // if not addressed yet, consider this command invalid
      // reconfigure after handshake
      if (valid_configuration_index(req->wValue)) {
        clear_setup();
        while (!(UEINTX & _BV(TXINI))) {}
        clear_in();
        while (!(UEINTX & _BV(TXINI))) {}
        // Set configuration *after* handshake
        set_configuration_index(req->wValue);
        if (req->wValue == 0) {
          set_device_state(DEFAULT);
        }
        else {
          set_device_state(CONFIGURED);
        }
      }
      break;
    case GET_DESCRIPTOR:
      // determine which descriptor to transmit and setup transaction
      if (req->bmRequestType == (REQ_DIR_IN | REQ_TYPE_STANDARD | REQ_REC_DEVICE)) {
        struct descriptor_list_t* head = generate_descriptor_list(req);
        struct descriptor_list_t* old_head;
        if (head) {
          clear_setup();

          while (!FLAG_IS_SET(UEINTX, TXINI)) {}
          // Send until requested size is reached
          // TODO Check if buffer is full and split transaction if needed
          uint16_t bytes_left = req->wLength;
          uint8_t body_length, header_bytes, body_bytes;
          while (head && bytes_left) {
            body_length = head->header.bLength - HEADER_SIZE;
            // Send header (if possible)
            header_bytes = bytes_left < HEADER_SIZE ? bytes_left : HEADER_SIZE;
            bytes_left -= fifo_write(&head->header, header_bytes);
            // Send body (if possible)
            body_bytes = bytes_left < body_length ? bytes_left : body_length;
            if (!head->body_in_flash) {
              bytes_left -= fifo_write(head->body, body_bytes);
            }
            else {
              bytes_left -= fifo_write_P(head->body, body_bytes);
            }
            // Proceed to next item and free current
            old_head = head;
            head = head->next;
            free(old_head);
          }
          clear_in();

          // Free any remaining descriptors
          while (head) {
            old_head = head;
            head = head->next;
            free(old_head);
          }

          // Wait for and clear handshake
          SET_FLAG(UEIENX, RXOUTE);
        }
      }
      break;
    default:
      break;
  }
}

#define VENDOR_REQUEST_PUSH_FRAME 1

static void process_vendor_request(const struct UsbSetupPacket_t* req) {
  if (req->bmRequestType == (REQ_DIR_OUT | REQ_TYPE_VENDOR | REQ_REC_INTERFACE)) {
    // If correct request and request length
    if (req->bRequest == VENDOR_REQUEST_PUSH_FRAME && req->wLength == FRAME_LENGTH) {
      struct frame_buffer_t* usb_frame = create_frame();
      if (usb_frame) {
        clear_setup();

        usb_frame->flags = FRAME_FREE_AFTER_DRAW;

        // Get frame data
        uint8_t* write_ptr = (uint8_t*) usb_frame->buffer;
        uint8_t* end_ptr = write_ptr + FRAME_LENGTH;
        while (write_ptr != end_ptr) {
          while (!(UEINTX & _BV(RXOUTI))) {}
          write_ptr += fifo_read(write_ptr, fifo_size());
          clear_out();
        }
        push_frame(usb_frame);

        // Handshake request
        while (!(UEINTX & _BV(TXINI))) {}
        clear_in();
      }
    }
  }
}

static void process_setup(const struct UsbSetupPacket_t* req) {
  switch (GET_REQUEST_TYPE(req->bmRequestType)) {
    case REQ_TYPE_STANDARD:
      process_standard_request(req);
      break;
    case REQ_TYPE_VENDOR:
      process_vendor_request(req);
      break;
    default:
      break;
  }
}

ISR(USB_COM_vect) {
  trip_led();
  struct UsbSetupPacket_t request;
  uint8_t prev_ep = UENUM;

  // Process USB transfers
  if (FLAG_IS_SET(UEINT, 0)) {
    UENUM = 0;
    if (FLAG_IS_SET(UEIENX, RXSTPE) && FLAG_IS_SET(UEINTX, RXSTPI)) {
      CLEAR_FLAG(UEIENX, RXOUTE);
      size_t read = fifo_read(&request, sizeof(struct UsbSetupPacket_t));
      if (read == sizeof(struct UsbSetupPacket_t)) {
        process_setup(&request);
      }

      if (FLAG_IS_SET(UEINTX, RXSTPI)) {
        SET_FLAG(UECONX, STALLRQ);
        CLEAR_INTERRUPT(UEINTX, RXSTPI);
      }
    }

    if (FLAG_IS_SET(UEIENX, TXINE) && FLAG_IS_SET(UEINTX, TXINI)) {
      // TODO Send remaining transaction data
      CLEAR_FLAG(UEIENX, TXINE);
      CLEAR_INTERRUPT(UEINTX, TXINI);
    }

    if (FLAG_IS_SET(UEIENX, RXOUTE) && FLAG_IS_SET(UEINTX, RXOUTI)) {
      // Acknowledge ZLP status handshake
      CLEAR_FLAG(UEIENX, RXOUTE);
      CLEAR_INTERRUPT(UEINTX, RXOUTI);
    }
  }

  // Restore endpoint number
  UENUM = prev_ep;
}
