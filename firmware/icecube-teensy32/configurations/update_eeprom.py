#!/usr/bin/python3
import struct
import json
import re
import hashlib

import logging

# Main logger
logger = logging.getLogger("icecube.LedDisplay")
logger.setLevel(logging.INFO)

handler = logging.StreamHandler()
handler.setFormatter(logging.Formatter("%(name)s [%(levelname)s] %(message)s"))
logger.addHandler(handler)

# EEPROM logger
logger = logging.getLogger("icecube.LedDisplay.eeprom")

import sys, os
sys.path.append(os.path.dirname(os.path.realpath(__file__))+"/../../../steamshovel")

from LedDisplay import DisplayController

def led_type_to_int(led_type):
  led_type = led_type.upper()
  if re.search("APA102[C]?", led_type):
    return DisplayController.LED_TYPE_APA102
  elif re.search("WS281[12][B]?", led_type):
    return DisplayController.LED_TYPE_WS2811
  else:
    return None

def led_order_to_int(order):
  order = order.upper()
  mapping = {
      "RGB" : 0
    , "BRG" : 1
    , "GBR" : 2
    , "BGR" : 3
    , "RBG" : 4
    , "GRB" : 5
  }
  if order in mapping:
    return mapping[order]
  else:
    return None

class SegmentConfiguration:
  # 15 character (+ null) utf-16-le string
  __OFFSET_SERIAL = 0
  __SERIAL = struct.Struct("<32s")
  # {led_type, color_order, string_start, string_stop, has_deepcore, reverse_first_strip_segment}
  __OFFSET_CONFIG = 0x20
  __CONFIG = struct.Struct("<BBBB??")
  # 4×(1+8) bytes string-to-strip mapping
  __OFFSET_PORT_MAP = 0x30
  __PORT_MAP = struct.Struct("<B8sB8sB8sB8s")
  # group identifier: binary encoded MD5 hash
  __OFFSET_GROUP_ID = 0x60
  __GROUP_ID = struct.Struct("<16s")

  def __init__(self, serial, name, led_config, string_config, other_serials=None):
    self.serial = serial
    self.name = name
    self.led_config = led_config
    self.string_config = string_config
    # Store sorted string list
    self._string_list_sorted = list()
    for port in self.string_config:
      self._string_list_sorted.extend(port)
    self._string_list_sorted.sort()

    self.__other_serials = other_serials

    if not self.validate_string_config():
      raise ValueError("Invalid string configuration")

  def __pack_serial(self):
    return self.__SERIAL.pack(self.serial.encode('utf-16-le'))

  def __pack_config(self):
    led_type = led_type_to_int(self.led_config['type'])
    led_order = led_order_to_int(self.led_config['order'])
    ic_string_min = self._string_list_sorted[0]

    i_max = len(self._string_list_sorted) - 1
    has_deepcore = self._string_list_sorted[i_max] > 78
    while self._string_list_sorted[i_max] > 78:
      i_max -= 1
    ic_string_max = self._string_list_sorted[i_max]

    if 'reverse_first' in self.led_config:
      reverse_first = bool(self.led_config['reverse_first'])
    else:
      reverse_first = True

    return self.__CONFIG.pack(
        led_type
      , led_order
      , ic_string_min
      , ic_string_max
      , has_deepcore
      , reverse_first
    )

  def __pack_port_map(self):
    buffer_offset_map = dict()
    for buffer_offset, string in enumerate(self._string_list_sorted):
      buffer_offset_map[string] = buffer_offset

    port_map_data = list()
    # Convert from {port : string} mapping to {strip segment : string} mapping
    for segment in range(4):
      # Determine number of ports with the current segment count by starting at the last
      # port and counting back until the number of segments on this port is at least the
      # current segment depth
      port_count = len(self.string_config)
      while port_count > 0 and len(self.string_config[port_count-1]) < segment+1:
        port_count -= 1

      port_map_data.append(port_count)

      segment_map = bytearray(8)
      for port in range(port_count):
        segment_map[port] = buffer_offset_map[self.string_config[port][segment]]

      port_map_data.append(bytes(segment_map))

    return self.__PORT_MAP.pack(*port_map_data)

  def __pack_group_id(self):
    serials = {self.serial}
    if self.__other_serials is not None:
      serials |= self.__other_serials

    serials = list(serials)
    serials.sort()
    identifier = '+'.join(serials)
    h = hashlib.md5(identifier.encode('utf-8'))
    return self.__GROUP_ID.pack(h.digest())

  def __update_eeprom(self, controller, offset, data):
    # Write and validate binary data
    data_original = controller.readEepromSegment(offset, len(data))

    if bytes(data_original) == bytes(data):
      logger.info(
        "EEPROM '{}' at offset 0x{:02x} already up-to-date".format(self.name, offset)
      )
      return

    controller.writeEepromSegment(offset, data)
    data_written = controller.readEepromSegment(offset, len(data))

    if data_written is None:
      logger.error("Could not read back EEPROM for verification")
    elif bytes(data) != bytes(data_written):
      logger.error("EEPROM was not written correctly")
    else:
      logger.info(
        "EEPROM '{}' at offset 0x{:02x} succesfully updated".format(self.name, offset)
      )
      printable_data = " ".join(map("{:02X}".format, data))
      logger.debug("Wrote data: " + printable_data)

  def write_eeprom_complete(self, controller):
    self.__update_eeprom(controller, self.__OFFSET_SERIAL, self.__pack_serial())
    self.__update_eeprom(controller, self.__OFFSET_CONFIG, self.__pack_config())
    self.__update_eeprom(controller, self.__OFFSET_GROUP_ID, self.__pack_group_id())
    self.write_string_config(controller)

  def write_string_config(self, controller):
    self.__update_eeprom(controller, self.__OFFSET_PORT_MAP, self.__pack_port_map())

  def validate_string_config(self):
    strings_set = set(self._string_list_sorted)
    if len(strings_set) != len(self._string_list_sorted):
      msg = "String mapping invalid: at least one string index used more than once"
      logger.error(msg)
      return False
    else:
      return True

  def validate(self, controller):
    strings_set = set(self._string_list_sorted)
    missing = set()
    for data_range in controller.data_ranges:
      range_set = set(range(data_range[0], data_range[1]+1))
      # If supported range is not a subset, determine strings missing from configuration
      # A non-empty set at the end of the loop indicates unmapped strings
      if not range_set <= strings_set:
        diff = range_set - strings_set
        missing |= diff

      # Remove supported strings from configuration strings
      # If this set is not empty, there are strings present that are not supported by the display
      strings_set -= range_set

    if len(missing) > 0:
      msg = "String(s) {} not in configuration for '{}'"
      logger.error(msg.format(sorted(missing), self.name))
      return False
    if len(strings_set) > 0:
      msg = "String(s) {} not supported by '{}'"
      logger.error(msg.format(sorted(strings_set), self.name))
      return False

    return True


class DisplayConfiguration:
  def __init__(self, config_file_path):
    self.segments = dict()
    self.configurations = dict()

    with open(config_file_path) as config_file:
      config = json.load(config_file)
      if "devices" not in config:
        raise ValueError("Configuration file does not contain 'devices' field")

      for serial in config["devices"]:
        segment_key = config["devices"][serial]
        if segment_key not in config:
          msg = "Invalid segment configuration '{}' for device '{}'".format(segment_key, serial)
          raise ValueError(msg)
        logger.debug("Found segment configuration '{}' for device '{}'".format(segment_key, serial))

        self.segments[serial] = SegmentConfiguration(
            serial
          , segment_key
          , config['led_config']
          , config[segment_key]
          , set(config["devices"].keys()) - {serial}
        )
        self.configurations[segment_key.lower()] = serial


if __name__ == "__main__":
  import argparse
  parser = argparse.ArgumentParser(description="Update IceCube display EEPROM")
  parser.add_argument("configuration_file", type=str, help="JSON file with display configuration")
  parser.add_argument("-f", "--full", action="store_true", help="Always ask to write the entire EEPROM")
  parser.add_argument("-v", "--verbose", action="store_true", help="Display the written EEPROM contents")
  args = parser.parse_args(sys.argv[1:])

  if args.verbose:
    logger.setLevel(logging.DEBUG)

  config = DisplayConfiguration(args.configuration_file)
  for controller in DisplayController.findAll():
    if args.full or len(controller.serial_number) == 0:
      if len(controller.serial_number) == 0:
        print("Found device without serial number")
        write_full = True
      else:
        print("Found device with current serial number {}".format(controller.serial_number))
        write_full = None
        while write_full is None:
          user_in = input("Overwrite existing EEPROM? [y/N]: ").lower()
          if len(user_in) == 0 or user_in[0] == 'n':
            write_full = False
          elif user_in[0] == 'y':
            write_full = True

      if write_full:
        print("Please select one of the available device configurations to configure the device:")
        sorted_serials = sorted(config.segments.keys())
        for serial in sorted_serials:
          print("    * {} (S/N {})".format(config.segments[serial].name, serial))
        selected = None
        while selected is None:
          selected = input("Select configuration: ").lower()
          if selected in config.configurations:
            serial = config.configurations[selected]
            segment = config.segments[serial]
            segment.write_eeprom_complete(controller)
            print("Configuration uploaded. Please mark the device to be able to match it with its")
            print("serial number ({}) or configuration ({}).".format(serial, segment.name))
          else:
            selected = None
            print("Invalid configuration! Please try again or exit to cancel.")

    elif controller.serial_number in config.segments:
      segment = config.segments[controller.serial_number]
      print("Found matching configuration for device '{}'".format(segment.serial))
      if not segment.validate(controller):
        continue

      do_update = input("Update string mapping? [Y/n]: ")
      if len(do_update) == 0 or do_update.lower()[0] == 'y':
        segment.write_string_config(controller)
        print("Please reboot the device for the new configuration to take effect.")

    else:
      print("Skipping device '{}' which is not present in configuration".format(controller.serial_number))
      print("Use the option --full to enable overwriting the full EEPROM contents")
