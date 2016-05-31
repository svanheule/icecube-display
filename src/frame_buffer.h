#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

/**
  * \file
  * \brief Description of the frame object.
  * \details
  * \author Sander Vanheule (Universiteit Gent)
  */

#include <stdint.h>

// Frame buffer size and structure definitions

/// Number of LEDs used in the display. This determines the frame buffer size.
#define LED_COUNT 78

/// A frame consist of LED_COUNT 4-tuples of bytes: brightness, red (R), green (G), blue (B)
/// The RGB values can be scaled using the brightness field to achieve a larger dynamic range,
/// e.g. to perform gamma correction.
struct led_t {
  uint8_t brightness;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

/// Total frame byte count
#define FRAME_LENGTH (sizeof(struct led_t)*LED_COUNT)

enum frame_flag_t {
  /// Indicate whether a frame buffer may be deallocated after drawing
  FRAME_FREE_AFTER_DRAW  = 1<<1,
  /// Indicate if the frame is currently being drawn.
  /// A renderer may choose to abstain from drawing to the buffer to avoid rendering artifacts.
  FRAME_DRAW_IN_PROGRESS = 1<<2
};

/** Object constisting of a frame buffer and a number of associated (bit)flags.
  * * flags(0): ::FRAME_FREE_AFTER_DRAW
  * * flags(1): ::FRAME_DRAW_IN_PROGRESS
  */
struct frame_buffer_t {
  struct led_t buffer[LED_COUNT];
  enum frame_flag_t flags;
};

/// Allocate a new frame buffer if possible. Returns NULL on failure.
struct frame_buffer_t* create_frame();

/// Deallocate a frame irrespective of whether the flag ::FRAME_FREE_AFTER_DRAW is set.
void destroy_frame(struct frame_buffer_t* frame);

/// Clear the frame contents, i.e. set `frame->buffer` to all zeros.
void clear_frame(struct frame_buffer_t* frame);
struct frame_buffer_t* create_empty_frame();

#endif
