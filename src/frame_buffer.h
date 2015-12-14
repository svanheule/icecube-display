
// Frame buffer size and structure definitions
// A frame consist of LED_COUNT 3-tuples of bytes, in the order: red, green, blue
// For example, `buffer[n][0]` is the red compoment of the n-th LED.
#define LED_COUNT 78
#define BUFFER_RED 0
#define BUFFER_GREEN 1
#define BUFFER_BLUE 2

// Total frame size
#define FRAME_LENGTH (LED_COUNT*3)

// Initialise the frame buffer to an empty display (all LEDs off)
void init_frame_buffer();

// Render a test frame based on an internal frame counter.
// Every consecutive frame will have the LED position shifted by one
void render_test_frame();
void render_ring();

// For use with the USART Rx ISR
void write_frame_byte(unsigned char word);

// Get the next frame to be written to the display
const unsigned char* const get_frame_buffer();

