#!/usr/bin/python3
import struct
import json

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
sys.path.append(os.path.dirname(os.path.realpath(__file__))+"/../steamshovel")

from LedDisplay import DisplayController


class DisplayConfiguration:
  __PORT_MAP = struct.Struct("<B8sB8sB8sB8s")
  __EEPROM_OFFSET = 0x30

  def __init__(self, config_file_path):
    self.string_configs = dict()
    self.segment_names = dict()

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

        self.string_configs[serial] = config[segment_key]
        self.segment_names[serial] = segment_key


  @classmethod
  def __pack_string_config(cls, string_config, buffer_offset_map):
    port_map_list = list()

    for segment in range(4):
      port_segment_count = len(string_config)
      port = len(string_config) - 1
      while port >= 0 and len(string_config[port])-1 < segment:
        port_segment_count -= 1
        port -= 1

      port_map_list.append(port_segment_count)

      port_segment_map = bytearray(8)
      for port in range(port_segment_count):
        port_segment_map[port] = buffer_offset_map[string_config[port][segment]]

      port_map_list.append(port_segment_map)

    return cls.__PORT_MAP.pack(*port_map_list)


  def update_devices(self):
    for controller in DisplayController.findAll():
      if controller.serial_number not in self.string_configs:
        msg = "--- Found no matching string configuration for device '{}'"
        logger.debug(msg.format(controller.serial_number))
        continue

      msg = "--- Found matching string configuration for device '{}'"
      logger.debug(msg.format(controller.serial_number))
      segment_name = self.segment_names[controller.serial_number]

      if controller.data_type != DisplayController.DATA_TYPE_IC_STRING:
        msg = "Device '{}' does not support correct data type"
        logger.error(msg.format(controller.serial_number))
        continue

      # Validate the configuration
      string_config = self.string_configs[controller.serial_number]
      strings = list()
      for port in string_config:
        strings.extend(port)

      strings_set = set(strings)
      if len(strings_set) != len(strings):
        msg = "String mapping invalid: at least one string index used more than once"
        logger.error(msg)
        continue

      buffer_offset_map = dict()
      offset = 0
      missing = set()
      for data_range in controller.data_ranges:
        # Determine buffer offsets
        for string in range(data_range[0], data_range[1]+1):
          buffer_offset_map[string] = offset
          offset += 1

        range_set = set(range(data_range[0], data_range[1]+1))
        # If supported range is not a subset, determine strings missing from configuration
        # A non-empty set at the end of the loop indicates unmapped strings
        if not range_set <= strings_set:
          diff = range_set - strings_set
          missing |= diff

        # Remove supported strings from configuration strings
        # If this set is not empty, there are strings present that are not supported by the display
        strings_set -= range_set

      if len(missing) > 0 or len(strings_set) > 0:
        if len(missing):
          msg = "String(s) {} not in configuration for '{}'"
          logger.error(msg.format(sorted(missing), segment_name))
        if len(strings_set):
          msg = "String(s) {} not supported by '{}'"
          logger.error(msg.format(sorted(strings_set), segment_name))
        continue

      data_original = controller.readEepromSegment(self.__EEPROM_OFFSET, self.__PORT_MAP.size)
      data_new = self.__pack_string_config(string_config, buffer_offset_map)

      if bytes(data_original) == bytes(data_new):
        logger.info("Configuration '{}' already up-to-date".format(segment_name))
        continue

      controller.writeEepromSegment(self.__EEPROM_OFFSET, data_new)
      data_written = controller.readEepromSegment(self.__EEPROM_OFFSET, self.__PORT_MAP.size)

      if data_written is None:
        logger.error("Could not read back EEPROM for verification")
      elif bytes(data_new) != bytes(data_written):
        logger.error("EEPROM was not written correctly")
      else:
        logger.info("Configuration '{}' succesfully updated".format(segment_name))


if __name__ == "__main__":
  import argparse
  parser = argparse.ArgumentParser(description="Update IceCube display string configuration")
  parser.add_argument("configuration_file", type=str)
  args = parser.parse_args(sys.argv[1:])

  config = DisplayConfiguration(args.configuration_file)
  config.update_devices()
