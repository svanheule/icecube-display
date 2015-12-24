#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include <stdint.h>

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

/**
 * The display driver uses double buffering with a front and back buffer.
 * The front buffer is used for displaying a frame, while the back buffer may be used to draw
 * new frame contents.
 */
/// Get the (read-only) pointer to the front buffer
const frame_t* get_front_buffer();
/// Get the pointer the back bufer for drawing.
frame_t* get_back_buffer();
/// Swap the front and back buffer after completing a frame draw.
void flip_pages();

/// Clear the frame contents, i.e. set all values to zero.
void clear_frame(frame_t* frame);

#endif
