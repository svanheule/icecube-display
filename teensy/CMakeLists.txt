# Cross-compiling can only be done with v2.6 or later
# Used generator expressions require v3.0
# CMake policy 0065 (disable -rdynamic) requires v3.4
cmake_minimum_required(VERSION 3.4)
project(ICECUBE_DISPLAY)

if(POLICY CMP0065)
  cmake_policy(SET CMP0065 NEW)
endif()

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER /usr/bin/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/arm-none-eabi-g++)

# Where is the target environment:
set(CMAKE_FIND_ROOT_PATH /usr/lib/arm-none-eabi)

# Search for programs in the build host directories:
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# For libraries and headers in the target directories:
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Device options
set(TARGET_DEVICE mk20dx256)
set(TARGET_F_CPU 48000000)
set(TARGET_F_LED   800000)

set(
  DEVICE_SERIAL "ICD-IC-001-0000"
  CACHE STRING "Device serial number, stored in the microcontroller's EEPROM"
)
set(
  DEVICE_ICECUBE_STRING_START "0"
  CACHE STRING "Number of the first normal IceCube string"
)
set_property(CACHE DEVICE_ICECUBE_STRING_START PROPERTY STRINGS "1" "31" "51")
set(
  DEVICE_ICECUBE_STRING_END "0"
  CACHE STRING "Number of the last normal IceCube string"
)
set_property(CACHE DEVICE_ICECUBE_STRING_END PROPERTY STRINGS "30" "50" "78")
set(
  DEVICE_HAS_DEEPCORE OFF
  CACHE BOOL "Whether this display segment contains the DeepCore strings"
)
set(TEST_MODE OFF CACHE BOOL "Run display in test mode")
set(DEVICE_FPS "1" CACHE STRING "Number of frames displayed per second")

# Set the GCC flags required
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -mthumb")
set(CMAKE_C_OUTPUT_EXTENSION ".o")
#set(CMAKE_VERBOSE_MAKEFILE true)

# Source files
include_directories(../src)
include_directories(src)
include_directories(cores/teensy3)
set(SOURCES
  src/main.c
  src/memspace.c
  # Frame management
  src/display_driver.c
  src/display_properties.c
  src/frame_buffer.c
  src/frame_queue.c
  src/frame_timer.c
  # Renderers
#  src/render/boot_splash.c
  src/render/test.c
#  src/render/demo.c
  # USB communication
  src/remote_usb.c
  src/usb/led.c
  src/usb/device.c
  src/usb/address.c
  src/usb/endpoint.c
  src/usb/endpoint_0.c
  src/usb/descriptor.c
  src/usb/configuration.c
  # Teensy specific
  src/kinetis/pit.c
  src/kinetis/ftm.c
  src/kinetis/dma.c
)
configure_file(src/usb/serial.c.in serial.c)
list(APPEND SOURCES "${CMAKE_BINARY_DIR}/serial.c")

# AVR compatibility source files from PJRC
list(APPEND SOURCES
  src/startup.c
  cores/teensy3/eeprom.c
)

set(PATH_LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/cores/teensy3)

# Build program and generate associated hex file
add_executable(icecube_display ${SOURCES})
set_target_properties(icecube_display PROPERTIES
  LINK_FLAGS "-Wl,-gc-sections,-no-wchar-size-warning,--section-start=.serialno=0x14000000,--section-start=.displayprop=0x14000020 -T ${PATH_LINKER_SCRIPT}/${TARGET_DEVICE}.ld --specs=nano.specs"
  SUFFIX ".elf"
)
target_compile_definitions(icecube_display
  PUBLIC __$<UPPER_CASE:${TARGET_DEVICE}>__
  PUBLIC F_CPU=${TARGET_F_CPU}UL
  PUBLIC F_LED=${TARGET_F_LED}UL
  PUBLIC DEVICE_ICECUBE_STRING_START=${DEVICE_ICECUBE_STRING_START}
  PUBLIC DEVICE_ICECUBE_STRING_END=${DEVICE_ICECUBE_STRING_END}
  PUBLIC DEVICE_HAS_DEEPCORE=$<BOOL:${DEVICE_HAS_DEEPCORE}>
  PUBLIC DEVICE_FPS=${DEVICE_FPS}
)
if(TEST_MODE)
  target_compile_definitions(icecube_display PUBLIC DEVICE_TEST_MODE)
endif()

target_compile_options(icecube_display
  PUBLIC -Wall -Wpedantic -Wshadow # Error messages
  PUBLIC -std=gnu11 # Language standard C11
  PUBLIC -Os -fshort-enums -ffunction-sections # Optimise for size
)

add_custom_command(
  TARGET icecube_display
  POST_BUILD
  COMMAND arm-none-eabi-objcopy -O ihex -R .eeprom -R .serialno -R .displayprop
        $<TARGET_FILE:icecube_display> icecube_display.hex
  COMMAND arm-none-eabi-objcopy -O ihex
        --change-section-lma .serialno=0x0 -j .serialno
        --change-section-lma .displayprop=0x20 -j .displayprop
        $<TARGET_FILE:icecube_display> icecube_display.eep
)

# Custom target to get size
add_custom_target(
  size
  arm-none-eabi-size $<TARGET_FILE:icecube_display>
  DEPENDS icecube_display
)

# Custom make target for uploading to microcontroller
set(
  PROGRAM_COMMAND
  ${CMAKE_SOURCE_DIR}/loader/teensy_loader_cli --mcu=$<UPPER_CASE:${TARGET_DEVICE}> -w
)

add_custom_target(
  upload
  ${PROGRAM_COMMAND} icecube_display.hex
  DEPENDS icecube_display
  COMMENT "Upload new program file"
)
