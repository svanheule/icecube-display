#ifndef RENDER_RENDERER_H
#define RENDER_RENDERER_H

/** \file
  * \brief Frame renderer interface.
  * \author Sander Vanheule (Universiteit Gent)
  */


/** \brief An object that returns frames indefinitely.
  * \details The renderer must be initialised by calling start() before using it,
  *   and should be deinitialised afterwards by calling stop().
  *   The behaviour of render_frame() is undefined before initalisation, and after deinitialisation.
  */
struct renderer_t {
  void (*start)(); ///< Initialise the renderer and allocate resources.
  void (*stop)(); ///< Deallocate resources.
  struct frame_buffer_t* (*render_frame)(); ///< Generate the next frame.
};

#endif // RENDER_RENDERER_H
