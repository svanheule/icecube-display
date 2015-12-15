// Initialise registers required for driving the display
void init_driver();

// Write a frame out to the display from the given frame buffer
void display_frame(const unsigned char *const buffer);
