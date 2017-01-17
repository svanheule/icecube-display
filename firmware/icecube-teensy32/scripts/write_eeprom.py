#!/usr/bin/python3
import usb.core
import usb.util

import struct
import json

import sys
import os

from intelhex import IntelHex

VENDOR_OUT_REQUEST = usb.util.build_request_type(
    usb.util.CTRL_OUT
  , usb.util.CTRL_TYPE_VENDOR
  , usb.util.CTRL_RECIPIENT_DEVICE
)
VENDOR_IN_REQUEST = usb.util.build_request_type(
    usb.util.CTRL_IN
  , usb.util.CTRL_TYPE_VENDOR
  , usb.util.CTRL_RECIPIENT_DEVICE
)
REQUEST_EEPROM_READ = 4
REQUEST_EEPROM_WRITE = 3

if len(sys.argv) < 2:
  print("Missing EEPROM hex-file path")
  sys.exit(-1)

eeprom_path = sys.argv[1]

if not os.path.isfile(eeprom_path):
  print("File not found '{}'".format(eeprom_path))
  sys.exit(-2)

device = usb.core.find(idVendor=0x1CE3, idProduct=2)
if device:
  device.set_configuration()
  serial = device.serial_number
  if len(serial) > 0:
    print("Programming device: {}".format(device.serial_number))
  else:
    print("Programming device: {}-{}".format(device.bus, device.address))

  eeprom_data = IntelHex()
  eeprom_data.fromfile(eeprom_path, format="hex")

  for (segment_start, segment_end) in eeprom_data.segments():
    segment_len = segment_end - segment_start
    segment_data = [eeprom_data[i] for i in range(segment_start, segment_end)]

    try:
      # Write new data
      sent = device.ctrl_transfer(
          VENDOR_OUT_REQUEST
        , REQUEST_EEPROM_WRITE
        , 0
        , segment_start
        , segment_data
        , timeout=500
      )

      device_data = device.ctrl_transfer(
          VENDOR_IN_REQUEST
        , REQUEST_EEPROM_READ
        , 0
        , segment_start
        , segment_len
        , timeout=500
      )

      # Check written data
      if bytes(device_data) != bytes(segment_data):
        print("Unable to write data segment")
      else:
        print("Segment (+{}, {} bytes) succesfully writen".format(segment_start, segment_len))

    except Exception as e:
      print("Could not open device: {}".format(e))

else:
  print("No compatible devices found")
