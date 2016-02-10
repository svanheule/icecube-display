# Cross-compiling can only be done with v2.6 or later
# Generator expressions require v2.8
cmake_minimum_required(VERSION 2.8)
project(POCKET_ICETOP)

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

option(ARDUINO "Build image for Arduino development hardware" OFF)

# Set the target AVR device
if(ARDUINO)
  set(AVR_TARGET_DEVICE atmega328p)
else()
  set(AVR_TARGET_DEVICE atmega32u4)
endif()

set(AVR_TARGET_F_CPU 16000000)

# Set the GCC flags required
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmcu=${AVR_TARGET_DEVICE}")
set(CMAKE_C_OUTPUT_EXTENSION ".o")
#set(CMAKE_VERBOSE_MAKEFILE true)

# Source files
include_directories(src)
set(SOURCES
  src/avr_main.c
  src/display_driver.c
  src/frame_buffer.c
  src/remote.c
  src/test_render.c
  src/demo.c
)

set(EVENTS
  event_s125_0_0.5_zen_0_0.1.txt
  event_s125_0_0.5_zen_0.2_0.25.txt
  event_s125_0_0.5_zen_0.3_0.35.txt
  event_s125_1_1.5_zen_0_0.1.txt
  event_s125_1_1.5_zen_0.2_0.25.txt
  event_s125_1_1.5_zen_0.3_0.35.txt
  event_s125_2_2.5_zen_0_0.1.txt
  event_s125_2_2.5_zen_0.2_0.25.txt
  event_s125_2_2.5_zen_0.3_0.35.txt
)

# Convert event TXT files to linkable object files
foreach(event ${EVENTS})
  set(event_bin_dir "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/events")
  if(NOT EXISTS ${event_bin_dir})
    file(MAKE_DIRECTORY ${event_bin_dir})
  endif()

  string(REPLACE ".txt" ".bin" bin_file_name ${event})
  set(event_file "${PROJECT_SOURCE_DIR}/events/${event}")
  set(bin_file "${event_bin_dir}/${bin_file_name}")
  set(object_file "${event_bin_dir}/${bin_file_name}.o")

  string(REGEX REPLACE "[^A-Za-z0-9]" "_" symbol_short ${bin_file_name})
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" symbol_long ${bin_file})

  add_custom_command(
    OUTPUT ${object_file}
    DEPENDS ${event_file} render.py
    COMMAND ./render.py -n -f ${event_file} -o ${bin_file}
    COMMAND avr-objcopy -I binary -O elf32-avr
      --redefine-sym _binary_${symbol_long}_start=_binary_${symbol_short}_start
      --redefine-sym _binary_${symbol_long}_end=_binary_${symbol_short}_end
      --redefine-sym _binary_${symbol_long}_size=_binary_${symbol_short}_size
      --rename-section .data=.progmem.data,contents,alloc,load,readonly,data
      "${bin_file}" "${object_file}"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )

  list(APPEND EVENT_OBJECTS ${object_file})
endforeach()

# Build program and generate associated hex file
add_executable(icetop_display ${SOURCES} ${EVENT_OBJECTS})
set_target_properties(icetop_display PROPERTIES
  LINK_FLAGS -Wl,-gc-sections
  SUFFIX ".elf"
)
target_compile_definitions(icetop_display
  PUBLIC F_CPU=${AVR_TARGET_F_CPU}UL
)
if(ARDUINO)
  target_compile_definitions(icetop_display PUBLIC CONTROLLER_ARDUINO)
else()
  target_compile_definitions(icetop_display PUBLIC CONTROLLER_UGENT)
endif()

target_compile_options(icetop_display
  PUBLIC -Wall -Wpedantic # Error messages
  PUBLIC -std=c99 # Language standard C99
  PUBLIC -Os -fshort-enums -ffunction-sections # Optimise for size
)
add_custom_command(
  TARGET icetop_display
  POST_BUILD
  COMMAND avr-objcopy -O ihex -R .eeprom $<TARGET_FILE:icetop_display> icetop_display.hex
)

# Custom make target for uploading to Arduino
if(ARDUINO)
  string(TOUPPER ${AVR_TARGET_DEVICE} target_upper)
  add_custom_target(
    upload
    avrdude -c arduino -p ${target_upper} -P /dev/ttyACM0 -b 115200 -U flash:w:icetop_display.hex
    DEPENDS icetop_display
    COMMENT "Upload new program file"
  )
endif()
