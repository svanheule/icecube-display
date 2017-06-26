#!/usr/bin/python3
import usb.core
import usb.util
import string

import sys, os
sys.path.append(os.path.dirname(os.path.realpath(__file__))+"/../steamshovel")

from LedDisplay import DisplayController

def single_width_printable(b):
  printable_chars = set(string.ascii_letters + string.digits + string.punctuation + ' ')
  b = chr(b)
  if b in printable_chars:
    return b
  else:
    return '·'

def print_bin_data(data):
  for line in range((len(data)+15)//16):
    offset = line*16
    data_line = data[line*16:(line+1)*16]
    data_bytes = ( "{:02X}".format(b) for b in data_line )
    data_printable = "".join(map(single_width_printable, data_line))
    print("  {:04X} | {:47s} | {:16s}".format(offset, " ".join(data_bytes), data_printable))

for controller in DisplayController.findAll():
  serial = controller.serial_number
  print("Querying device: {}".format(serial))

  if len(sys.argv) > 1:
    eeprom_len = int(sys.argv[1])
  elif serial.startswith("ICD-IT-001"):
    eeprom_len = 0x20 + 0x10
  elif serial.startswith("ICD-IC-001"):
    eeprom_len = 0x20 + 0x10 + 0x30 + 0x10
  else:
    eeprom_len = 4096

  try:
    data = controller.readEepromSegment(0, eeprom_len)

    print("  Read {} bytes".format(len(data)))
    print_bin_data(data)

  except usb.core.USBError as e:
    print("  Error reading information: {}".format(e))

