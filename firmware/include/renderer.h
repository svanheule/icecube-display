#ifndef RENDER_RENDERER_H
#define RENDER_RENDERER_H

/** \file
  * \brief Frame renderer interface.
  * \author Sander Vanheule (Universiteit Gent)
  *
  * \defgroup display_renderer Display renderers
  * \brief Display content rendering
  * \details For stand-alone operation an device testing, a frame renderer interface was developed.
  *   Depending on the device state, one renderer is selected and used to generate frames that are
  *   pushed into the frame queue.
  *   Every time a frame is consumed, the current renderer's renderer_t::render_frame() function
  *   is called to generate the next frame.
  */


/** \brief An object that returns frames indefinitely.
  * \details The renderer must be initialised by calling start() before using it,
  *   and should be deinitialised afterwards by calling stop().
  *   The behaviour of render_frame() is undefined before initalisation, and after deinitialisation.
  * \ingroup display_renderer
  */
struct renderer_t {
  void (*start)(); ///< Initialise the renderer and allocate resources.
  void (*stop)(); ///< Deallocate resources.
  struct frame_buffer_t* (*render_frame)(); ///< Generate the next frame.
};

#endif // RENDER_RENDERER_H
