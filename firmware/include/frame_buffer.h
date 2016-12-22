#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

/**
  * \file
  * \brief Description of the frame object.
  * \author Sander Vanheule (Universiteit Gent)
  *
  * \defgroup led_display LED display buffer usage
  * \brief LED display buffer implementation and usage.
  * \details The LED display consists of a number of integrated LED modules driven by serial
  *   communication. A frame buffer therefore contains the display data, plus one byte of flags.
  *   A pool of memory with room for multiple frames is pre-allocated and calling create_frame()
  *   will mark one the available frame buffers as used and return a pointer to it. This pointer
  *   can then be used to draw new frame contents, push it into the frame queue for display and
  *   release the memory with destroy_frame() when it is no longer of use.
  *
  *   frame_buffer_t::buffer has room for as many bytes as required by the display.
  *   A display with for example 78 APA102 LEDs will have a buffer size of \f$312=78\times4\f$.
  *   The contents of the buffer are stored in OM-key order. This means that LED data is
  *   first sorted by string number, and then by DOM number.
  *   The buffer data however will not be written to the display in-order, as following the
  *   buffer layout for the physical layout of LEDs is usually subobtimal.
  *   See the display documentation for more information on how the LEDs are physically connected.
  *
  *   RGB data for the LEDs is always stored in this order, independent of the data format taken
  *   by the LED modules, to simplify the PC driver code.
  *   In case of the APA102 modules, an extra brightness byte `b` is required. This is stored
  *   _before_ the other data, resulting in a `bRGB` data pattern.
  *
  *   Two flags are currently supported as defined by ::frame_flag_t. A newly allocated frame will
  *   not have any of these set, so the user should take care of setting these as needed to prevent
  *   any memory leaks or corruption. After drawing a frame with its ::FRAME_FREE_AFTER_DRAW flag
  *   set, the memory will be released. Using this pointer after the frame has been released, may
  *   result in memory corruption, so take care not to used dangling pointers!
  *   In the current implementation, frame memory is not dynamically allocated and will eventually
  *   be used to draw other frame contents. If two renderers were to render to the same memory
  *   region, the frame may contain contents of both renderers. Since the memory is not used by
  *   any other code however, using invalid pointers will most likely not crash the code, but only
  *   result in odd things being displayed. The same odd-behaviour-warning applies to a frame whose
  *   ::FRAME_DRAW_IN_PROGRESS flag is set. Since a frame buffer is pushed to the LED string
  *   out-of-order, weird tearing effects may occur.
  */

#include <stdint.h>
#include <stddef.h>
#include "display_properties.h"

// Frame buffer size and structure definitions

/// Total frame byte count
/// \ingroup led_display
size_t get_display_buffer_size();

/// Initialise data storage for display frames.
void init_display_buffers();

/// Constants that can be used as metadata bit flags on a frame buffer.
/// \ingroup led_display
enum frame_flag_t {
  /// Indicate whether a frame buffer may be deallocated after drawing
  FRAME_FREE_AFTER_DRAW  = 1<<1,
  /// Indicate if the frame is currently being drawn.
  /// A renderer may choose to abstain from drawing to the buffer to avoid rendering artifacts.
  FRAME_DRAW_IN_PROGRESS = 1<<2
};

/// \brief Object constisting of a frame buffer and a number of associated (bit)flags.
/// \ingroup led_display
struct frame_buffer_t {
  /// Frame metadata as bit flags (see ::frame_flag_t)
  /// * flags(0): ::FRAME_FREE_AFTER_DRAW
  /// * flags(1): ::FRAME_DRAW_IN_PROGRESS
  enum frame_flag_t flags;
  /// Frame buffer LED data.
  uint8_t* buffer;
};

/// \name Frame buffer handling
/// @{

/// Allocate a new frame buffer if possible. Returns NULL on failure.
/// \ingroup led_display
struct frame_buffer_t* create_frame();

/// Deallocate a frame irrespective of whether the flag ::FRAME_FREE_AFTER_DRAW is set.
/// \ingroup led_display
void destroy_frame(struct frame_buffer_t* frame);

/// Clear the frame contents, i.e. set `frame->buffer` to all zeros.
/// \ingroup led_display
void clear_frame(struct frame_buffer_t* frame);

/** \brief Convenience method to create a new frame of which frame_buffer_t::buffer is set to
  * all zeros.
  * \details Identical to calling clear_frame() on a pointer returned by create_frame().
  * Note that *no flags will be set* on the newly created buffer.
  * \ingroup led_display
  */
struct frame_buffer_t* create_empty_frame();

/// @}

#endif
