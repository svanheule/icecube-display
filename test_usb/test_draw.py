#!/usr/bin/python3
# -*- coding: utf-8 -*-

import logging
logger = logging.getLogger("icecube.LedDisplay")
logger.setLevel(logging.INFO)
logger.addHandler(logging.StreamHandler())

from time import sleep, time

import sys, os
sys.path.append(os.path.dirname(os.path.realpath(__file__))+"/../steamshovel")

from LedDisplay import DisplayManager, DisplayController

manager = DisplayManager()

if len(sys.argv) > 1:
  offset = int(sys.argv[1])
else:
  offset = 0

import colorsys

#
# Draw an HSV loop to the display
#

if len(manager.displays) and offset:
  transmission_durations = list()
  for display in manager.displays:
    transmission_durations.append(list())

  while offset:
    logger.debug("{} frames to go".format(offset))
    time_start = time()
    for i,display in enumerate(manager.displays):
      pixel_size = display.led_class.DATA_LENGTH
      rgb_to_data = display.led_class.float_to_led_data
      buffer_length = display.buffer_length
      frame = bytearray(buffer_length)

      if display.data_type == DisplayController.DATA_TYPE_IC_STRING:
        pixel_offset = (offset-1)%60
        for pixel in range(60):
          hue = (pixel-pixel_offset)/60
          if hue < 0:
            hue += 1
          data = rgb_to_data(colorsys.hls_to_rgb(hue, 0.1, 1.0))
          for string_offset in range(display.string_count):
            buffer_offset = string_offset*60+pixel_offset
            frame[buffer_offset*pixel_size:(buffer_offset+1)*pixel_size] = data

      elif display.data_type == DisplayController.DATA_TYPE_IT_STATION:
        pixel_offset = (offset-1)%display.string_count
        for pixel in range(display.string_count):
          hue = (pixel-pixel_offset)/display.string_count
          if hue < 0:
            hue += 1
          data = rgb_to_data(colorsys.hls_to_rgb(hue, 0.1, 1.0))
          buffer_offset = pixel_size*pixel_offset
          frame[pixel*pixel_size:(pixel+1)*pixel_size] = data

      frame = bytes(frame)

      transfer_start = time()
      display.transmitDisplayBuffer(frame)
      transfer_stop = time()
      transmission_durations[i].append(transfer_stop-transfer_start)
 
    offset -= 1
    sleep(max(0, 1./25 - (time()-time_start)))
  
  for i,display in enumerate(manager.displays):
    logger.debug("Clearing display")
    clear_frame = bytearray(display.buffer_length)
    display.transmitDisplayBuffer(clear_frame)
    tx = transmission_durations[i]
    logger.info("Mean TX duration: {} ms".format(sum(tx)/len(tx)*1000))

