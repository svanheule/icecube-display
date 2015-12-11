#include "frame_buffer.h"

// Double frame buffer
static volatile unsigned char frame_buffer_0[LED_COUNT][3];
static volatile unsigned char frame_buffer_1[LED_COUNT][3];

// TODO Triple buffering with round-robin buffer usage

// Current frame writing state state
enum RxState_t {STATE_WAIT, STATE_RECEIVING};
typedef enum RxState_t RxState;

struct FrameRxState_t {
  RxState rx_state;
  unsigned char* write_ptr;
  unsigned char* next_end;
};
typedef struct FrameRxState_t FrameRxState;

static volatile FrameRxState frame_rx_state;
static volatile unsigned char* current_frame_ptr;
static volatile unsigned char* next_frame_ptr;


static void clear_frame(unsigned char* const frame_ptr) {
  unsigned char* write_ptr = frame_ptr;
  unsigned char* frame_end = frame_ptr + FRAME_LENGTH;

  while (write_ptr != frame_end) {
    *(++write_ptr) = 0;
  }
}


void init_frame_buffer() {
  current_frame_ptr = frame_buffer_0;
  next_frame_ptr = frame_buffer_1;

  clear_frame(current_frame_ptr);
  clear_frame(next_frame_ptr);

  frame_rx_state.rx_state = STATE_WAIT;
}


static unsigned char frame_count;

void render_test_frame() {
  // Clear the frame
  clear_frame(next_frame_ptr);
  current_frame_ptr[frame_count, BUFFER_RED] = 0xFF;
  current_frame_ptr[frame_count, BUFFER_GREEN] = 0xFF;
  current_frame_ptr[frame_count, BUFFER_BLUE] = 0xFF;
  frame_count = (frame_count + 1)%LED_COUNT;

  unsigned char* tmp = current_frame_ptr;
  current_frame_ptr = next_frame_ptr;
  next_frame_ptr = tmp;
}


void write_frame_byte(unsigned char word) {

  switch (frame_rx_state.rx_state) {
    case STATE_WAIT:
      // Set up new pointers
      frame_rx_state.write_ptr = next_frame_ptr;
      frame_rx_state.next_end = next_frame_ptr + FRAME_LENGTH;
      // Write first byte and transition to writing frame
      *(frame_rx_state.write_ptr++) = word;
      frame_rx_state.rx_state = STATE_RECEIVING;
      break;

    case STATE_RECEIVING:
      // Write byte
      *(frame_rx_state.write_ptr++) = word;
      // If this write completed the frame, swap frame pointers and return to STATE_WAIT
      if (frame_rx_state.write_ptr == frame_rx_state.next_end) {
        // Swap current and next frame pointers
        unsigned char* tmp = next_frame_ptr;
        next_frame_ptr = current_frame_ptr;
        current_frame_ptr = tmp;
        // Transition back to waiting
        frame_rx_state.rx_state = STATE_WAIT;
      }
      break;

    default:
      break;
  }
}

const unsigned char* const get_frame_buffer() {
  return current_frame_ptr;
}

