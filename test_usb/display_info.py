#!/usr/bin/python3
import struct
import binascii

import sys, os
sys.path.append(os.path.dirname(os.path.realpath(__file__))+"/../steamshovel")

from LedDisplay import DisplayController


def formatTlvField(field):
  t, l, v = field
  # Information type
  if t == DisplayController.DP_TYPE_INFORMATION_TYPE and l == 1:
    if v[0] == DisplayController.DATA_TYPE_IT_STATION:
      return "Display shows IceTop stations"
    elif v[0] == DisplayController.DATA_TYPE_IC_STRING:
      return "Display shows IceCube strings"
    else:
      return "Unknown information type"
  # Information range
  elif t == DisplayController.DP_TYPE_INFORMATION_RANGE and l == 2:
    return "Supported information range: {}-{}".format(v[0], v[1])
  # Led type
  elif t == DisplayController.DP_TYPE_LED_TYPE and l == 1:
    led = "unknown"
    if v[0] == DisplayController.LED_TYPE_APA102:
      led = "APA102"
    elif v[0] == DisplayController.LED_TYPE_WS2811:
      led = "WS2811/WS2812"
    return "Supported LED data format: {}".format(led)
  elif t == DisplayController.DP_TYPE_END:
    return "[end of display properties]"
  elif t == DisplayController.DP_TYPE_BUFFER_SIZE:
    return "Display buffer size: {}".format(struct.unpack("<H", v[:2])[0])
  else:
    return "Unknown field ({}) or invalid length ({}): {}".format(t, l, binascii.hexlify(v))


for controller in DisplayController.findAll():
  print("Querying device: {}".format(controller.serial_number))

  try:
    for field in controller.readDisplayInfo():
      print("  * {}".format(formatTlvField(field)))

  except Exception as e:
    print("Error reading information: {}".format(e))
