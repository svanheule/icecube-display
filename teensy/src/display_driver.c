#include <stdint.h>

#include <kinetis.h>
#include "core_cm4.h"

#define DMAMEM __attribute__ ((section(".dmabuffers"), used))

// TODO Define in CMake/makefile
#define F_LED 800000

struct transfer_control_descriptor_t {
  const volatile void* SADDR;
  int16_t SOFF;
  union {
    uint16_t ATTR;
    struct {
      uint8_t ATTR_DST;
      uint8_t ATTR_SRC;
    };
  };
  union {
    uint32_t NBYTES;
    uint32_t NBYTES_MLNO;
    uint32_t NBYTES_MLOFFNO;
    uint32_t NBYTES_MLOFFYES;
  };
  int32_t SLAST;
  volatile void* DADDR;
  int16_t DOFF;
  union {
    uint16_t CITER;
    uint16_t CITER_ELINKYES;
    uint16_t CITER_ELINKNO;
  };
  int32_t DLASTSGA;
  uint16_t CSR;
  union {
    uint16_t BITER;
    uint16_t BITER_ELINKYES;
    uint16_t BITER_ELINKNO;
  };
} __attribute__((packed));

static volatile struct transfer_control_descriptor_t* const transfer_control_descriptor =
    (volatile struct transfer_control_descriptor_t* const) 0x40009000;

static void clear_channel_tcd(const uint8_t channel) {
  transfer_control_descriptor[channel] = (struct transfer_control_descriptor_t) {
    .SADDR=0,
    .SOFF=0,
    .ATTR=0,
    .NBYTES=0,
    .SLAST=0,
    .DADDR=0,
    .DOFF=0,
    .CITER=0,
    .DLASTSGA=0,
    .CSR=0,
    .BITER=0
  };
}

struct ftm_channel_config_t {
  uint32_t SC;
  uint32_t VAL;
} __attribute__((packed));

struct ftm_config_t {
  uint32_t SC;
  uint32_t CNT;
  uint32_t MOD;
  struct ftm_channel_config_t channels[8];
  uint32_t CNTIN;
  uint32_t STATUS;
  uint32_t MODE;
  uint32_t SYNC;
  uint32_t OUTINIT;
  uint32_t OUTMASK;
  uint32_t COMBINE;
  uint32_t DEADTIME;
  uint32_t EXTTRIG;
  uint32_t POL;
  uint32_t FMS;
  uint32_t FILTER;
  uint32_t FLTCTRL;
  uint32_t QDCTRL;
  uint32_t CONF;
  uint32_t FLTPOL;
  uint32_t SYNCONF;
  uint32_t INVCTRL;
  uint32_t SWOCTRL;
  uint32_t PWMLOAD;
} __attribute__((packed));

static volatile struct ftm_config_t* const ftm_config = (volatile struct ftm_config_t*) 0x400B8000;

#define STRING_LENGTH 60
#define MAX_PORT_COUNT 8
#define SEGMENT_COUNT 4

#define BUFFER_STEP sizeof(struct led_t)
#define STRIP_LENGTH (sizeof(struct led_t)*STRING_LENGTH)

static uint8_t led_data[STRIP_LENGTH*(3*8)] DMAMEM;
static const uint32_t len = sizeof(led_data);

static uint8_t ones = 0xFF;

// OctoWS2811 init_display_driver
void init_display_driver() {
  /** Based on OctoWS2811 code **/

  // Initialise output port
  SIM_SCGC5 |= _BV(12); // Enable port D clock
  GPIOD_PDDR = 0xFF; // Set all port pins as output
  PORTD_GPCHR = (0xFF<<16); // Disable all interrupts on port D
  PORTD_GPCLR = (0xFF<<16) | (1<<8); // Select ALT1 (GPIO) mode for all pins
  GPIOD_PCOR = 0xFF; // Clear all pin outputs

  // Initialise DMA
  SIM_SCGC6 |= _BV(1); // SIM_SCGC6(1) DMA mux module
  SIM_SCGC7 |= _BV(1); // SIM_SCGC7(1) DMA module

  DMA_CR = _BV(7);
  DMA_ERQ = 0;
  DMA_EEI = 0;
  clear_channel_tcd(0);
  clear_channel_tcd(1);
  clear_channel_tcd(2);

  // Set all outputs high at start of pulse
  transfer_control_descriptor[0].CSR = _BV(3);
  transfer_control_descriptor[0].SADDR = &ones;
  transfer_control_descriptor[0].DADDR = &GPIOD_PSOR;
  transfer_control_descriptor[0].NBYTES = 1;
  transfer_control_descriptor[0].BITER = len;
  transfer_control_descriptor[0].CITER = len;

  // Write bit values after time_0_high
  // SADDR, SOFF, SLAST and DADDR are set when initiating a frame write
  transfer_control_descriptor[1].CSR = _BV(3);
  transfer_control_descriptor[1].NBYTES = 1;
  transfer_control_descriptor[1].BITER = len;
  transfer_control_descriptor[1].CITER = len;

  // Set all outputs low after time_1_high
  transfer_control_descriptor[2].CSR = _BV(3) | _BV(1);
  transfer_control_descriptor[2].SADDR = &ones;
  transfer_control_descriptor[2].DADDR = &GPIOD_PCOR;
  transfer_control_descriptor[2].NBYTES = 1;
  transfer_control_descriptor[2].BITER = len;
  transfer_control_descriptor[2].CITER = len;

  // Disable used DMA channels
  DMAMUX0_CHCFG0 = 0;
  DMAMUX0_CHCFG2 = 0;
  DMAMUX0_CHCFG1 = 0;

  SIM_SCGC3 |= _BV(24); // SIM_SCGC3(24) for FTM2

  const uint16_t counter_total = (F_BUS / F_LED);
  const uint16_t count_0_high = 0.4/1.25*(F_BUS/F_LED);
  const uint16_t count_1_high = 0.8/1.25*(F_BUS/F_LED);

  ftm_config->SC = 0;
  ftm_config->MOD = counter_total-1;

  ftm_config->channels[0].SC = 0;
  ftm_config->channels[1].SC = 0;

  ftm_config->channels[0].VAL = count_0_high-1;
  ftm_config->channels[1].VAL = count_1_high-1;

  // Configure port B(18) as FTM2_CH0 output to be able to trigger a DMA request on its rising edge
  PORTB_PCR18 = (1<<16) | (3<<8);

  // Enable DMA channels
  DMAMUX0_CHCFG0 = 50 | _BV(7); // Ch 50 = port B
  DMAMUX0_CHCFG1 = 34 | _BV(7); // Ch 34 = FTM2_CH0
  DMAMUX0_CHCFG2 = 35 | _BV(7); // Ch 34 = FTM2_CH1

  // TODO Read color order and determine pointer differences
}

void dma_ch2_isr() {
  // Clear the interrupt
  DMA_CINT = 2;
  // Halt the FTM clock and disable this IRQ
  ftm_config->SC = 0;
  NVIC_DISABLE_IRQ(IRQ_DMA_CH2);

  // Set the outputs low if for whatever reason they aren't already.
  // This ensures that the LED string receives a RESET signal and
  // the next frame will be displayed properly.
  GPIOD_PDOR = 0;
}

static void start_dma_transfer() {
  // Disable clock and clear counter
  ftm_config->SC = 0;
  ftm_config->CNT = 0;

  // Disable DMA channels and clear pending interrupts
  DMA_ERQ = 0;
  DMA_CINT = _BV(6);

  // Interrupt enable, Edge-aligned PWM w/ clear output on match
  const uint32_t ftm_channel_dma_requests = _BV(6)|_BV(0);
  const uint32_t ftm_channel_output = _BV(5)|_BV(3);
  const uint32_t ftm_channel_interrupt_mask = _BV(7);

  // Disable DMA request interrupts
  ftm_config->channels[0].SC = ftm_channel_output;
  ftm_config->channels[1].SC = ftm_channel_output;

  // Set FTM outputs to initial value (0's)
  ftm_config->MODE |= _BV(1);

  // Clear possible pending DMA requests to make sure they happen in the right order
  for (unsigned i = 0; i < 2; ++i) {
    ftm_config->channels[i].SC &= ~ftm_channel_interrupt_mask;
    ftm_config->channels[i].SC = ftm_channel_output | fmt_channel_dma_requests;
  }
  PORTB_ISFR = _BV(18);

  // Enable DMA interrupts for channel 2 and enable DMA channels
  NVIC_ENABLE_IRQ(IRQ_DMA_CH2);
  DMA_ERQ = _BV(2) | _BV(1) | _BV(0);

  // Start transfer by selecting FTM clock
  ftm_config->SC = (1<<3);
}


// LED strip to buffer offset mapping
// Map format: STRIP[7:0] := string number
struct port_map_t {
  uint8_t ports[MAX_PORT_COUNT];
};

static const struct port_map_t LED_MAP_FRONT[SEGMENT_COUNT] = {
    {{7, 8, 15, 14, 11, 19, 27, 18}}
  , {{6, 22, 16, 2, 10, 26, 12, 28}}
  , {{0, 3, 24, 21, 4, 20, 29, 25}}
  , {{1, 9, 23, 13, 5, 0, 0, 17}}
};
// TODO Define other led geometry mappings

static const struct port_map_t* led_mapping = &(LED_MAP_FRONT[0]);

struct led_t {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} __attribute__((packed));

// Store a 8b×8b matrix as two 32b little-endian integers
union matrix_t {
  uint8_t rows[8];
  struct {
    uint32_t low;
    uint32_t high;
  };
};

/* Swap bits in a 32b word using XOR operations.
 * For the two set of bits have that are to be swapped, the first one has to be right shifted on the
 * position of the second one. The mask then indicates the position of the bits in the second set.
 * Note that the mask must necessarily also be a shifted version the locations of the first bit set.
 * Example: To swap bits (31, 20, 15, 13) with bits (18, 7, 2, 0) provide the following values:
 *  - `shift`: `13`
 *  - `mask`: `0x00040085` (`0b00000000_00000100_00000000_10000101`)
 */
static inline uint32_t swap_bits(uint32_t rows, uint8_t shift, uint32_t mask) {
  uint32_t swapped_bits = (rows ^ (rows >> shift)) & mask;
  return rows ^ swapped_bits ^ (swapped_bits << shift);
}

union matrix_t transpose_matrix(union matrix_t m) {
  /* Transposing a matrix can be done recursively for matrices of size (2^n × 2^n).
   * 1. Divide the matrix T into four submatrices T[i,j], each of size (2^(n-1) × 2^(n-1)).
   * 2. Swap T[0,1] and T[1,0].
   * 3. Repeat steps 1 and 2 for each submatrix until the submatrices have reached size (1×1).
   *
   * The following algorithm is applied recursively to swap (blocks of) high and low bits.
   * Steps 2 and 3 can be combined into one expression for swap_level_2 and swap_level_3.
   * 1. XOR high bits that into low bits, and mask out all other data
   * 2. XOR original high data with temp to cancel original low bits and keep high bits
   * 3. XOR original low data with shifted temp to cancel original high bits and keep low bits
   *
   * Bytes can be kept in little-endian representation by rotating around j=7-i axis.
   * This is equivalent to reversing the byte order before and after the transpose (with axis i=j).
   *
   * The original implementation this was based can be found at:
   *   http://www.hackersdelight.org/hdcodetxt/transpose8.c.txt
   */

  // Stage 1 swap: 4 (4×4) submatrices
  uint32_t swapped_bits = ((m.high >> 4) ^ m.low) & 0x0F0F0F0F;
  m.high = m.high ^ (swapped_bits << 4);
  m.low = m.low ^ swapped_bits;

  // Stage 2 swap: 16 (2×2) submatrices
  m.high = swap_bits(m.high, 18, 0x00003333);
  m.low = swap_bits(m.low, 18, 0x00003333);

  // Stage 3 swap: 64 (1×1) submatrices
  m.high = swap_bits(m.high, 9, 0x00550055);
  m.low = swap_bits(m.low, 9, 0x00550055);

  return m;
}

#define OFFSET_R ((ptrdiff_t) offsetof(struct led_t, r))
#define OFFSET_G ((ptrdiff_t) offsetof(struct led_t, g))
#define OFFSET_B ((ptrdiff_t) offsetof(struct led_t, b))

// Color order is currently hard-coded GRB
static ptrdiff_t delta_0 = OFFSET_R - OFFSET_G;
static ptrdiff_t delta_1 = OFFSET_B - OFFSET_R;

static void copy_buffer(const void* restrict src, void* restrict dest) {
  // Perform a linear write to the output buffer, at the expense of having to jump around
  // the input buffer *a lot*.
  union matrix_t* output = (union matrix_t*) dest;

  const ptrdiff_t color_offset_initial = OFFSET_G;
  const ptrdiff_t color_offset_rewind = -(delta_0 + delta_1);
  ptrdiff_t delta_color_offset[sizeof(struct led_t)] = {delta_0, delta_1, 0};

  // Current reading positions for all ports
  const uint8_t* input[MAX_PORT_COUNT];
  // Current last position for port 0; all ports need the same amount of data any way.
  const uint8_t* input_0_end;

  for (unsigned int segment = 0; segment < SEGMENT_COUNT; ++segment) {
    // If even segment, LED data is in reverse order as DOM numbering starts at the top of a string
    const bool is_odd = segment % 2;

    const uint8_t* initial_position = ((const uint8_t*) src) + color_offset_initial;
    if (!is_odd) {
      // Pointer is always 'increment/decrement after', so initial value cannot be past-the-end
      initial_position += STRIP_LENGTH - BUFFER_STEP;
    }

    for (unsigned int port = 0; port < MAX_PORT_COUNT; ++port) {
      const uint8_t string = led_mapping[segment].ports[port];
      input[port] = initial_position + STRIP_LENGTH*string;
    }

    if (is_odd) {
      input_0_end = input[0] + STRIP_LENGTH;
      delta_color_offset[2] = color_offset_rewind + BUFFER_STEP;
    }
    else {
      input_0_end = input[0] - STRIP_LENGTH;
      delta_color_offset[2] = color_offset_rewind - BUFFER_STEP;
    }

    // Shuffle LED data from USB buffer format to OctoWS2811 format
    while (input[0] != input_0_end) {
      for (unsigned int color = 0; color < sizeof(struct led_t); ++color) {
        const ptrdiff_t offset_next = delta_color_offset[color];
        // Gather data for all ports
        for (unsigned int port = 0; port < MAX_PORT_COUNT; ++port) {
          // Copy 8 data bytes for the current color
          output->rows[port] = *input[port];
          // Jump to next color (possibly of the next LED)
          input[port] += offset_next;
        }

        // Transpose bytes to correct output format
        *output = transpose_matrix(*output);
        output++;
      }
    }
  }
}

void display_frame(struct frame_buffer_t* buffer) {
  copy_buffer(buffer->buffer, &(led_data[0]));

  // Setup TCD to write buffer data
  transfer_control_descriptor[1].SADDR = &(led_data[0]);
  transfer_control_descriptor[1].SOFF = 1;
  transfer_control_descriptor[1].SLAST = -len;
  transfer_control_descriptor[1].DADDR = &GPIOD_PDOR;

  start_dma_transfer();
}

void display_blank() {
  // Setup TCD to write blank data
  transfer_control_descriptor[1].SADDR = &ones;
  transfer_control_descriptor[1].SOFF = 0;
  transfer_control_descriptor[1].SLAST = 0;
  transfer_control_descriptor[1].DADDR = &GPIOD_PCOR;

  start_dma_transfer();
}

