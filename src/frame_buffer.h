#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

// Frame buffer size and structure definitions
// A frame consist of LED_COUNT 4-tuples of bytes: brightness, red, green, blue
#define LED_COUNT 78

struct led_t {
  uint8_t brightness;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

// Array of led_t objects, so (frame_t) ~ (led_t*)
typedef struct led_t frame_t[LED_COUNT];
// Total frame size
#define FRAME_LENGTH sizeof(frame_t)

// Frame queue
/// Flag to indicate a frame buffer may be deallocated after drawing
#define FRAME_FREE_AFTER_DRAW 1

/// Flag to indicate if the frame is currently being drawn.
/// A renderer may choose to abstain from drawing to the buffer to avoid rendering artifacts.
#define FRAME_DRAW_IN_PROGRESS (1<<1)

/// Object constisting of a frame buffer and a number of associated (bit)flags.
/// * flags(0): FRAME_FREE_AFTER_DRAW
/// * flags(1): FRAME_DRAW_IN_PROGRESS
struct frame_buffer_t {
  frame_t buffer;
  uint8_t flags;
};

struct frame_buffer_t* create_frame();

void destroy_frame(struct frame_buffer_t* frame);

/// Clear the frame contents, i.e. set all values to zero.
void clear_frame(struct frame_buffer_t* frame);

bool frame_queue_full();
bool frame_queue_empty();

bool push_frame(struct frame_buffer_t* frame);
struct frame_buffer_t* pop_frame();

/// A renderer is an object that, once initialised/started, must return frames indefinitely.
struct renderer_t {
  void (*start)();
  void (*stop)();
  struct frame_buffer_t* (*render_frame)();
};
#endif
