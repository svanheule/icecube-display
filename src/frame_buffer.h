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

/** \brief A 4-tuple of bytes describing the data required by the APA102 LEDs.
  * \details The 24-bit RGB values can be scaled using the brightness field to achieve
  *   a larger dynamic range, e.g. to perform gamma correction.
  */
struct led_t {
  uint8_t brightness; ///< Global brightness bits; only 5 LSB are valid.
  uint8_t red; ///< 8 bit red component.
  uint8_t green; ///< 8 bit green component.
  uint8_t blue; ///< 8 bit blue component.
};

/// Total frame byte count
#define FRAME_LENGTH (sizeof(struct led_t)*LED_COUNT)

/// Constants that can be used as metadata bit flags on a frame buffer.
enum frame_flag_t {
  /// Indicate whether a frame buffer may be deallocated after drawing
  FRAME_FREE_AFTER_DRAW  = 1<<1,
  /// Indicate if the frame is currently being drawn.
  /// A renderer may choose to abstain from drawing to the buffer to avoid rendering artifacts.
  FRAME_DRAW_IN_PROGRESS = 1<<2
};

/// Object constisting of a frame buffer and a number of associated (bit)flags.
struct frame_buffer_t {
  /// Frame buffer LED data.
  struct led_t buffer[LED_COUNT];
  /// Frame metadata as bit flags (see ::frame_flag_t)
  /// * flags(0): ::FRAME_FREE_AFTER_DRAW
  /// * flags(1): ::FRAME_DRAW_IN_PROGRESS
  enum frame_flag_t flags;
};

/// Allocate a new frame buffer if possible. Returns NULL on failure.
struct frame_buffer_t* create_frame();

/// Deallocate a frame irrespective of whether the flag ::FRAME_FREE_AFTER_DRAW is set.
void destroy_frame(struct frame_buffer_t* frame);

/// Clear the frame contents, i.e. set `frame->buffer` to all zeros.
void clear_frame(struct frame_buffer_t* frame);

/// Convenience method to create a new frame of which frame_buffer_t::buffer is set to all zeros.
/// Identical to calling clear_frame() on a pointer returned by create_frame().
/// Note that *no flags will be set* on the newly created buffer.
struct frame_buffer_t* create_empty_frame();

#endif
