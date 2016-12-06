#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include <stdatomic.h>

#include "kinetis/io.h"
#include "kinetis/ftm.h"
#include "kinetis/dma.h"
#include <avr/eeprom.h>

#include "display_driver.h"
#include "display_properties.h"


// Buffer dimensions
#define STRING_LENGTH 60
#define SEGMENT_COUNT 4
#define STRIP_LENGTH (STRING_LENGTH*SEGMENT_COUNT)

#define MAX_PORT_COUNT 8

#define BUFFER_STEP sizeof(struct led_t)
#define BUFFER_SIZE (MAX_PORT_COUNT*STRIP_LENGTH*sizeof(struct led_t))

// Color order
#define OFFSET_RED ((ptrdiff_t) offsetof(struct led_t, red))
#define OFFSET_GREEN ((ptrdiff_t) offsetof(struct led_t, green))
#define OFFSET_BLUE ((ptrdiff_t) offsetof(struct led_t, blue))

static ptrdiff_t color_offset_initial;
static ptrdiff_t delta_0;
static ptrdiff_t delta_1;

// LED strip to buffer offset mapping
struct port_map_t {
  uint8_t ports_length;
  uint8_t ports[MAX_PORT_COUNT];
};

// TODO Revise LED layout: see with other groups on used layouts
#define PORTMAP __attribute__((section(".portmap"),used))
static const struct port_map_t LED_MAP[SEGMENT_COUNT] PORTMAP;

static struct port_map_t led_mapping[SEGMENT_COUNT];

static volatile atomic_flag frame_write_in_progress;

// DMA sources
#define DISPMEM __attribute__ ((section(".displaybuffer")))
static alignas(4) uint8_t ones DISPMEM;
static alignas(4) uint8_t led_data[BUFFER_SIZE] DISPMEM;

// Defaoult FTM channel configuration
static const uint32_t ftm_channel_output = _BV(5)|_BV(3);

// OctoWS2811 init_display_driver
void init_display_driver() {
  /** Based on OctoWS2811 code **/

  // Initialise data output port
  ATOMIC_REGISTER_BIT_SET(SIM_SCGC5, 12); // Enable port D clock
  PORTD_GPCLR = (0xFFFF<<16) | (1<<8); // Select ALT1 (GPIO) mode for all pins
  PORTD_GPCHR = (0xFFFF<<16); // Disable all interrupts on port D
  GPIOD_PDDR = 0xFF; // Set all port pins as output
  GPIOD_PCOR = 0xFF; // Clear all pin outputs

  // Initialise DMA
  ATOMIC_REGISTER_BIT_SET(SIM_SCGC6, 1); // SIM_SCGC6(1) DMA mux module
  ATOMIC_REGISTER_BIT_SET(SIM_SCGC7, 1); // SIM_SCGC7(1) DMA module

  DMA_CR = _BV(7);
  DMA_ERQ = 0;
  DMA_EEI = 0;
  clear_channel_tcd(0);
  clear_channel_tcd(1);
  clear_channel_tcd(2);

  // Initialise ones to 0xFF since the .displaybuffer section is not initialised
  ones = 0xFF;

  // Set all outputs high at start of pulse
  dma_tcd_list[0].CSR = _BV(3);
  dma_tcd_list[0].SADDR = &ones;
  dma_tcd_list[0].DADDR = &GPIOD_PSOR;
  dma_tcd_list[0].NBYTES = 1;
  dma_tcd_list[0].BITER = BUFFER_SIZE;
  dma_tcd_list[0].CITER = BUFFER_SIZE;

  // Write bit values after time_0_high
  // SADDR, SOFF, SLAST and DADDR are set when initiating a frame write
  dma_tcd_list[1].CSR = _BV(3);
  dma_tcd_list[1].NBYTES = 1;
  dma_tcd_list[1].BITER = BUFFER_SIZE;
  dma_tcd_list[1].CITER = BUFFER_SIZE;

  // Set all outputs low after time_1_high
  dma_tcd_list[2].CSR = _BV(3) | _BV(1);
  dma_tcd_list[2].SADDR = &ones;
  dma_tcd_list[2].DADDR = &GPIOD_PCOR;
  dma_tcd_list[2].NBYTES = 1;
  dma_tcd_list[2].BITER = BUFFER_SIZE;
  dma_tcd_list[2].CITER = BUFFER_SIZE;

  // Disable used DMA channels
  DMAMUX0_CHCFG0 = 0;
  DMAMUX0_CHCFG2 = 0;
  DMAMUX0_CHCFG1 = 0;

  ATOMIC_REGISTER_BIT_SET(SIM_SCGC3, 24); // SIM_SCGC3(24) for FTM2

  const uint16_t counter_total = (F_BUS / F_LED);
  const uint16_t count_0_high = 0.4/1.25*(F_BUS/F_LED);
  const uint16_t count_1_high = 0.8/1.25*(F_BUS/F_LED);

  ftm2_config->SC = 0;
  ftm2_config->MOD = counter_total-1;

  ftm2_config->channels[0].SC = ftm_channel_output;
  ftm2_config->channels[1].SC = ftm_channel_output;

  ftm2_config->channels[0].VAL = count_0_high-1;
  ftm2_config->channels[1].VAL = count_1_high-1;

  // Configure port B(18) as FTM2_CH0 output to be able to trigger a DMA request on its rising edge
  ATOMIC_REGISTER_BIT_SET(SIM_SCGC5, 10);
  PORTD_GPCHR = (0xF<<16); // Disable all interrupts on port B
  PORTB_PCR18 = (1<<16) | (3<<8);

  // Enable DMA channels
  DMAMUX0_CHCFG0 = 50 | _BV(7); // Ch 50 = port B
  DMAMUX0_CHCFG1 = 34 | _BV(7); // Ch 34 = FTM2_CH0
  DMAMUX0_CHCFG2 = 35 | _BV(7); // Ch 34 = FTM2_CH1

  // Read color order and determine pointer differences
  switch (get_color_order()) {
    case LED_ORDER_BGR:
      color_offset_initial = OFFSET_BLUE;
      delta_0 = OFFSET_GREEN-OFFSET_BLUE;
      delta_1 = OFFSET_RED-OFFSET_GREEN;
      break;
    case LED_ORDER_BRG:
      color_offset_initial = OFFSET_BLUE;
      delta_0 = OFFSET_RED-OFFSET_BLUE;
      delta_1 = OFFSET_GREEN-OFFSET_RED;
      break;
    case LED_ORDER_GBR:
      color_offset_initial = OFFSET_GREEN;
      delta_0 = OFFSET_BLUE-OFFSET_GREEN;
      delta_1 = OFFSET_RED-OFFSET_BLUE;
      break;
    case LED_ORDER_GRB:
      color_offset_initial = OFFSET_GREEN;
      delta_0 = OFFSET_RED-OFFSET_GREEN;
      delta_1 = OFFSET_BLUE-OFFSET_RED;
      break;
    case LED_ORDER_RBG:
      color_offset_initial = OFFSET_RED;
      delta_0 = OFFSET_BLUE-OFFSET_RED;
      delta_1 = OFFSET_GREEN-OFFSET_BLUE;
      break;
    case LED_ORDER_RGB:
    default:
      color_offset_initial = OFFSET_RED;
      delta_0 = OFFSET_GREEN-OFFSET_RED;
      delta_1 = OFFSET_BLUE-OFFSET_GREEN;
      break;
  }

  eeprom_read_block(&led_mapping, &LED_MAP, sizeof(LED_MAP));
  for (unsigned int segment = 0; segment < SEGMENT_COUNT; ++segment) {
    if (led_mapping[segment].ports_length > MAX_PORT_COUNT) {
      led_mapping[segment].ports_length = 0;
    }
  }

  // PDB configuration for frame reset timer
  ATOMIC_REGISTER_BIT_SET(SIM_SCGC6, 22); // Enable PDB clock
  PDB0_SC = PDB_SC_PDBIE | PDB_SC_TRGSEL(15);
  NVIC_ENABLE_IRQ(IRQ_PDB);
  // 50 µs delay (equiv 20000/s) with 48MHz F_BUS: 2400 counts (no prescaler required)
  const uint16_t reset_delay_counts = (F_BUS/20000);
  PDB0_MOD = reset_delay_counts - 1;
  PDB0_IDLY = reset_delay_counts - 1;

  PDB0_SC = PDB_SC_PDBIE | PDB_SC_TRGSEL(15) | PDB_SC_LDOK | PDB_SC_PDBEN;

  atomic_flag_clear(&frame_write_in_progress);
}

void dma_ch2_isr() {
  // Clear the interrupt
  DMA_CINT = 2;
  // Halt the FTM clock and disable this IRQ
  ftm2_config->SC = 0;
  NVIC_DISABLE_IRQ(IRQ_DMA_CH2);

  // Set the outputs low if for whatever reason they aren't already.
  // This ensures that the LED string receives a RESET signal and
  // the next frame will be displayed properly.
  GPIOD_PDOR = 0;

  // Trigger 50µs delay timer
  ATOMIC_REGISTER_BIT_SET(PDB0_SC, 16);
}

void pdb_isr() {
  ATOMIC_REGISTER_BIT_CLEAR(PDB0_SC, 6);
  atomic_flag_clear(&frame_write_in_progress);
}

static void start_dma_transfer() {
  // Disable clock and clear counter
  ftm2_config->SC = 0;
  ftm2_config->CNT = 0;

  // Disable DMA channels and clear pending interrupts
  DMA_ERQ = 0;
  DMA_CINT = _BV(6);

  // Interrupt enable, Edge-aligned PWM w/ clear output on match
  const uint32_t ftm_channel_dma_requests = _BV(6)|_BV(0);
  const uint32_t ftm_channel_interrupt_mask = _BV(7)|_BV(6);

  // Disable DMA request interrupts
  ftm2_config->channels[0].SC = ftm_channel_output;
  ftm2_config->channels[1].SC = ftm_channel_output;

  // Set FTM outputs to initial value (0's)
  ATOMIC_REGISTER_BIT_SET(ftm2_config->MODE, 1);

  // Clear possible pending DMA requests to make sure they happen in the right order
  for (unsigned i = 0; i < 2; ++i) {
    // Clear interrupt flag with read-modify-write cycle
    ftm2_config->channels[i].SC &= ~ftm_channel_interrupt_mask;
    ftm2_config->channels[i].SC = ftm_channel_output | ftm_channel_dma_requests;
  }
  PORTB_ISFR = _BV(18);

  // Enable DMA interrupts for channel 2 and enable DMA channels
  NVIC_ENABLE_IRQ(IRQ_DMA_CH2);
  DMA_ERQ = _BV(2) | _BV(1) | _BV(0);

  // Start transfer by selecting FTM clock
  ftm2_config->SC = (1<<3);
}

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

static union matrix_t transpose_matrix(union matrix_t m) {
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


static void copy_buffer(const void* restrict src, void* restrict dest) {
  // Perform a linear write to the output buffer, at the expense of having to jump around
  // the input buffer *a lot*.
  union matrix_t* output = (union matrix_t*) dest;

  const ptrdiff_t color_offset_rewind = -(delta_0 + delta_1);
  ptrdiff_t delta_color_offset[sizeof(struct led_t)] = {delta_0, delta_1, 0};

  // Current reading positions for all ports
  const uint8_t* input[MAX_PORT_COUNT];
  // Current last position for port 0; all ports need the same amount of data any way.
  const uint8_t* input_0_end;

  unsigned int segment = 0;
  while (segment < SEGMENT_COUNT && led_mapping[segment].ports_length > 0) {
    const uint8_t used_port_count = led_mapping[segment].ports_length;
    // If even segment, LED data is in reverse order as DOM numbering starts at the top of a string
    const bool is_odd = segment % 2;

    const uint8_t* initial_position = ((const uint8_t*) src) + color_offset_initial;
    if (!is_odd) {
      // Pointer is always 'increment/decrement after', so initial value cannot be past-the-end
      initial_position += (STRING_LENGTH-1)*BUFFER_STEP;
    }

    for (unsigned int port = 0; port < used_port_count; ++port) {
      const uint8_t string = led_mapping[segment].ports[port];
      input[port] = initial_position + STRING_LENGTH*BUFFER_STEP*string;
    }

    if (is_odd) {
      input_0_end = input[0] + STRING_LENGTH*BUFFER_STEP;
      delta_color_offset[2] = color_offset_rewind + BUFFER_STEP;
    }
    else {
      input_0_end = input[0] - STRING_LENGTH*BUFFER_STEP;
      delta_color_offset[2] = color_offset_rewind - BUFFER_STEP;
    }

    // Shuffle LED data from USB buffer format to OctoWS2811 format
    while (input[0] != input_0_end) {
      for (unsigned int color = 0; color < sizeof(struct led_t); ++color) {
        // Gather data for all ports
        union matrix_t m;

        for (unsigned int port = 0; port < used_port_count; ++port) {
          // Copy 8 data bytes for the current color
          m.rows[MAX_PORT_COUNT-1 - port] = *input[port];
          // Jump to next color (possibly of the next LED)
          input[port] += delta_color_offset[color];
        }

        // Transpose bytes to correct output format
        *output = transpose_matrix(m);
        output++;
      }
    }

    segment++;
  }
}

void display_frame(struct frame_buffer_t* buffer) {
  if (!atomic_flag_test_and_set(&frame_write_in_progress)) {
    ATOMIC_SRAM_BIT_SET(buffer->flags, 2);
    copy_buffer(buffer->buffer, &(led_data[0]));
    ATOMIC_SRAM_BIT_CLEAR(buffer->flags, 2);

    // Setup TCD to write buffer data
    dma_tcd_list[1].SADDR = &(led_data[0]);
    dma_tcd_list[1].SOFF = 1;
    dma_tcd_list[1].SLAST = -BUFFER_SIZE;
    dma_tcd_list[1].DADDR = &GPIOD_PDOR;

    start_dma_transfer();
  }
}

void display_blank() {
  if (!atomic_flag_test_and_set(&frame_write_in_progress)) {
    // Setup TCD to write blank data
    dma_tcd_list[1].SADDR = &ones;
    dma_tcd_list[1].SOFF = 0;
    dma_tcd_list[1].SLAST = 0;
    dma_tcd_list[1].DADDR = &GPIOD_PCOR;

    start_dma_transfer();
  }
}

