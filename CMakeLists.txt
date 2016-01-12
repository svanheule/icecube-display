# Cross-compiling can only be done with v2.6 or later
cmake_minimum_required(VERSION 2.6)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

set(CMAKE_C_COMPILER /usr/bin/avr-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/avr-g++)

# Where is the target environment:
set(CMAKE_FIND_ROOT_PATH /usr/lib/avr)

# Search for programs in the build host directories:
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# For libraries and headers in the target directories:
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Workaround so --export_dynamic isn't passed to linker
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# Set the target AVR device
set(AVR_TARGET_DEVICE atmega328p)
set(AVR_TARGET_F_CPU 16000000)

# Set the GCC flags required
set(CMAKE_C_FLAGS "-mmcu=${AVR_TARGET_DEVICE}")
set(CMAKE_C_OUTPUT_EXTENSION ".o")
#set(CMAKE_VERBOSE_MAKEFILE true)

include_directories(src)
set(SOURCES
  src/avr_main.c
  src/display_driver.c
  src/frame_buffer.c
  src/serial.c
  src/test_render.c
  src/demo.c
)

# Build program and generate associated hex file
add_executable(icetop_display ${SOURCES})
set_target_properties(icetop_display PROPERTIES
  LINK_FLAGS -Wl,-gc-sections
  SUFFIX ".elf"
)
target_compile_definitions(icetop_display
  PUBLIC F_CPU=${AVR_TARGET_F_CPU}UL
)
target_compile_options(icetop_display
  PUBLIC -Wall -Wpedantic -std=gnu99 -Os -fshort-enums -ffunction-sections
)
add_custom_command(
  TARGET icetop_display
  POST_BUILD
  COMMAND avr-objcopy -O ihex -R .eeprom $<TARGET_FILE:icetop_display> icetop_display.hex
)

# Custom make target for uploading to Arduino
add_custom_target(
  upload
  avrdude -c arduino -p ATMEGA328P -P /dev/ttyACM0 -b 115200 -U flash:w:icetop_display.hex
  DEPENDS icetop_display
  COMMENT "Upload new program file"
)
