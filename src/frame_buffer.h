#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

/**
  * \file
  * \brief Description of the frame object and a FIFO queue to use as buffer.
  * \details
  * \author Sander Vanheule (Universiteit Gent)
  */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

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

// Frame queue
enum frame_flag_t {
  /// Indicate whether a frame buffer may be deallocated after drawing
  FRAME_FREE_AFTER_DRAW  = 1,
  /// Indicate if the frame is currently being drawn.
  /// A renderer may choose to abstain from drawing to the buffer to avoid rendering artifacts.
  FRAME_DRAW_IN_PROGRESS = 2
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

/** Deallocate a frame if the ::FRAME_FREE_AFTER_DRAW flag is set.
  * This function is a no-op if this is not the case.
  */
void destroy_frame(struct frame_buffer_t* frame);

/// Clear the frame contents, i.e. set all values to zero.
void clear_frame(struct frame_buffer_t* frame);

/// Check if the frame queue is full.
bool frame_queue_full();
/// Check if the frame queue is empty.
bool frame_queue_empty();

/** Push new frame into the frame FIFO.
  * If the queue is already full, destroy_frame() will be called.
  * If ::FRAME_FREE_AFTER_DRAW is set, this means that ownership of the frame is transferred to the
  * queue. If it is not set, the frame's memory will not be released and the pointer can be
  * re-used.
  * \returns `true` on success, and `false` if the FIFO was full.
  */
bool push_frame(struct frame_buffer_t* frame);

/** Pop a frame from the frame FIFO.
  * \returns Pointer to the popped frame, or NULL if the FIFO was empty.
  */
struct frame_buffer_t* pop_frame();

/** A renderer is an object that, once initialised/started, must return frames indefinitely.
  * `start` (`stop`) should be called when (de)initialising the object. The behaviour of
  * `render_frame` is undefined before calling `start`, and after calling `stop`.
  */
struct renderer_t {
  void (*start)();
  void (*stop)();
  struct frame_buffer_t* (*render_frame)();
};

#endif
