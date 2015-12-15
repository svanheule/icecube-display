#include "frame_buffer.h"
#include <math.h>

// Double frame buffer
static unsigned char frame_buffer_0[FRAME_LENGTH];
static unsigned char frame_buffer_1[FRAME_LENGTH];

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

static FrameRxState frame_rx_state;
static unsigned char* current_frame_ptr;
static unsigned char* next_frame_ptr;


static void clear_frame(unsigned char* frame_ptr) {
  unsigned char* frame_end = frame_ptr + FRAME_LENGTH;

  while (frame_ptr != frame_end) {
    *(frame_ptr++) = 0;
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

// Render consecutive concentric rings around pixel (5,4), i.e. station 36
static const signed char STATION_VECTOR[LED_COUNT][2] = {
    {5,0}, {4,0}, {3,0}, {2,0}, {1,0}, {0,0}
  , {0,1}, {1,1}, {2,1}, {3,1}, {4,1}, {5,1}, {6,1}
  , {7,2}, {6,2}, {5,2}, {4,2}, {3,2}, {2,2}, {1,2}, {0,2}
  , {0,3}, {1,3}, {2,3}, {3,3}, {4,3}, {5,3}, {6,3}, {7,3}, {8,3}
  , {9,4}, {8,4}, {7,4}, {6,4}, {5,4}, {4,4}, {3,4}, {2,4}, {1,4}, {0,4}
  , {1,5}, {2,5}, {3,5}, {4,5}, {5,5}, {6,5}, {7,5}, {8,5}, {9,5}, {10,5}
  , {10,6}, {9,6}, {8,6}, {7,6}, {6,6}, {5,6}, {4,6}, {3,6}, {2,6}
  , {3,7}, {4,7}, {5,7}, {6,7}, {7,7}, {8,7}, {9,7}, {10,7}
  , {10,8}, {9,8}, {8,8}, {7,8}, {6,8}, {5,8}, {4,8}
  , {5,9}, {6,9}, {7,9}, {8,9}
};
static char ring_frame;
typedef signed char Station[2];

static unsigned char station_distance(Station s1, Station s2) {
  signed char dv = s1[0]-s2[0];
  signed char dw = s1[1]-s2[1];
  return abs(dv) + abs(dw) + abs(dv+dw);
}

void render_ring() {
  unsigned char station;
  for (station = 0; station < LED_COUNT; ++station) {
    unsigned char d = distance(STATION_VECTOR[station], STATION_VECTOR[35]);
    if (d == ring_frame) {
      // Red ring
      next_frame_ptr[3*station+BUFFER_RED] = 0x10;
      next_frame_ptr[3*station+BUFFER_GREEN] = 0x00;
      next_frame_ptr[3*station+BUFFER_BLUE] = 0x00;
    }
    else if (d+1 == ring_frame) {
      // Green ring
      next_frame_ptr[3*station+BUFFER_RED] = 0x00;
      next_frame_ptr[3*station+BUFFER_GREEN] = 0x10;
      next_frame_ptr[3*station+BUFFER_BLUE] = 0x00;
    }
    else if (d+2 == ring_frame) {
      // Blue ring
      next_frame_ptr[3*station+BUFFER_RED] = 0x00;
      next_frame_ptr[3*station+BUFFER_GREEN] = 0x00;
      next_frame_ptr[3*station+BUFFER_BLUE] = 0x10;
    }
    else {
      next_frame_ptr[3*station+BUFFER_RED] = 0x00;
      next_frame_ptr[3*station+BUFFER_GREEN] = 0x00;
      next_frame_ptr[3*station+BUFFER_BLUE] = 0x00;
    }
  }

  // There are 6 rings, so expand beyond
  ring_frame = (ring_frame+1)%8;

  // Swap frames
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
        unsigned char* tmp = current_frame_ptr;
        current_frame_ptr = next_frame_ptr;
        next_frame_ptr = tmp;
        // Transition back to waiting
        frame_rx_state.rx_state = STATE_WAIT;
      }
      break;

    default:
      break;
  }
}

const unsigned char *const get_frame_buffer() {
  return current_frame_ptr;
}

