# Cross-compiling can only be done with v2.6 or later
# Used generator expressions require v3.0
cmake_minimum_required(VERSION 3.0)
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
option(FUSES "Enable setting of fuses and lock bits (advanced feature, may brick your device)" OFF)

# Set the target AVR device
if(ARDUINO)
  set(AVR_TARGET_DEVICE atmega328p)
  # Fuse values, default Arduino values
  set(FUSE_LOW 0xff)
  set(FUSE_HIGH 0xde)
  set(FUSE_EXTENDED 0x05)
  set(LOCK 0xcf)
  # Programmer: Arduino on port /dev/ttyACM0, baudrate 115200
  set(DEFAULT_PROGRAMMER_ID arduino)
  set(DEFAULT_PROGRAMMER_PORT_FLAGS -P /dev/ttyACM0 -b 115200)
else()
  set(AVR_TARGET_DEVICE atmega32u4)
  # Fuse values
  # >8MHz crystal, low rise VCC, no clock-out, no clock divide by 8
  set(FUSE_LOW 0xff)
  # no on-chip debug, no JTAG, enable SPI, no WDT, EEPROM save, bootsize 2k, boot to application
  set(FUSE_HIGH 0xd1)
  # BOD level 4.3V, HWB/DFU enable
  set(FUSE_EXTENDED 0xc0)
  # Lock bits: prevent firmware from writing to in bootloader section
  set(LOCK 0x2f)
  set(DEFAULT_PROGRAMMER_ID flip1)
endif()

set(AVR_TARGET_F_CPU 16000000)
set(
  PROGRAMMER_ID ${DEFAULT_PROGRAMMER_ID}
  CACHE STRING "Programmer used to upload the application with avrdude"
)
set(
  PROGRAMMER_PORT_FLAGS ${DEFAULT_PROGRAMMER_PORT_FLAGS}
  CACHE STRING "avrdude options to specify the programmer settings"
)
set(
  DEVICE_SERIAL "ICD-IT-001-0000"
  CACHE STRING "Device serial number, stored in the microcontroller's EEPROM"
)

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
  src/test_render.c
  src/demo.c
  src/display_state.c
  src/switches.c
)

if(ARDUINO)
  list(APPEND SOURCES
    src/remote_serial.c
  )
else()
  list(APPEND SOURCES
    src/remote_usb.c
    src/usb/isr.c
    src/usb/led.c
    src/usb/device.c
    src/usb/endpoint.c
    src/usb/fifo.c
    src/usb/descriptor.c
    src/usb/configuration.c
  )
  configure_file(src/usb/serial.c.in serial.c)
  list(APPEND SOURCES "${CMAKE_BINARY_DIR}/serial.c")
endif()

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
  set_property(SOURCE ${object_file} PROPERTY EXTERNAL_OBJECT TRUE)
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
  PUBLIC -Wall -Wpedantic -Wshadow # Error messages
  PUBLIC -std=gnu11 # Language standard C11
  PUBLIC -Os -fshort-enums -ffunction-sections # Optimise for size
)
add_custom_command(
  TARGET icetop_display
  POST_BUILD
  COMMAND avr-objcopy -O ihex -R .eeprom $<TARGET_FILE:icetop_display> icetop_display.hex
  COMMAND avr-objcopy -O ihex --change-section-lma .eeprom=0 -j .eeprom
        $<TARGET_FILE:icetop_display> icetop_display.eep
)

# Custom target to get size
add_custom_target(
  size
  avr-size -C --mcu=${AVR_TARGET_DEVICE} $<TARGET_FILE:icetop_display>
  DEPENDS icetop_display
)

# Custom make target for uploading to microcontroller
if(NOT ("${PROGRAMMER_ID}" STREQUAL ""))
  set(
    PROGRAM_COMMAND
    avrdude -p $<UPPER_CASE:${AVR_TARGET_DEVICE}> -c ${PROGRAMMER_ID} ${PROGRAMMER_PORT_FLAGS}
  )

  add_custom_target(
    upload
    ${PROGRAM_COMMAND} -U flash:w:icetop_display.hex
    DEPENDS icetop_display
    COMMENT "Upload new program file"
  )

  # Custom target for setting the fuses
  if(FUSES)
    add_custom_target(
      fuses
      ${PROGRAM_COMMAND}
        -U lfuse:w:${FUSE_LOW}:m
        -U hfuse:w:${FUSE_HIGH}:m
        -U efuse:w:${FUSE_EXTENDED}:m
      COMMENT "Program microcontroller fuses"
    )

    add_custom_target(
      lock
      ${PROGRAM_COMMAND} -U lock:w:${LOCK}:m
      COMMENT "Program memory lock bits"
    )
  endif()

  add_custom_target(
    upload_eeprom
    ${PROGRAM_COMMAND} -D -U eeprom:w:icetop_display.eep
    DEPENDS icetop_display
    COMMENT "Upload device EEPROM"
  )

  # Custom target to flash, and download if needed, the Atmel DFU bootloader
  if(NOT ARDUINO)
    set(bootloader_dir "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/images")
    set(bootloader_zip_filename "megaUSB_DFU_Bootloaders.zip")
    set(bootloader_hex_filename "ATMega32U4-usbdevice_dfu-1_0_0.hex")
    set(bootloader_url "http://www.atmel.com/Images/${bootloader_zip_filename}")
    set(bootloader_zip "${bootloader_dir}/${bootloader_zip_filename}")
    set(bootloader_hex "${bootloader_hex_filename}")

    if(NOT EXISTS ${bootloader_dir})
      file(MAKE_DIRECTORY ${bootloader_dir})
    endif()

    if(NOT EXISTS ${bootloader_zip})
      file(
        DOWNLOAD ${bootloader_url} ${bootloader_zip}
        EXPECTED_HASH SHA256=252947050b9f6623001cc3d0c8d4b2a14398ed22bebc249cb9c1abc8d0db1608
      )
    endif()

    add_custom_target(
      bootloader_image
      ${CMAKE_COMMAND} -E tar xf ${bootloader_zip} -- ${bootloader_hex_filename}
      DEPENDS ${bootloader_zip}
    )

    add_custom_target(
      bootloader
      ${PROGRAM_COMMAND} -U flash:w:${bootloader_hex}
      DEPENDS bootloader_image
      COMMENT "Upload DFU bootloader"
    )

    add_custom_target(
      erase_device
      ${PROGRAM_COMMAND} -e
      COMMENT "Erase device memory"
    )
  endif()

endif()
