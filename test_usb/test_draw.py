#!/usr/bin/python3
import usb.core
import usb.util
import struct
import sys

from time import sleep, time

in_request = usb.util.build_request_type(
    usb.util.CTRL_IN
  , usb.util.CTRL_TYPE_VENDOR
  , usb.util.CTRL_RECIPIENT_DEVICE
)
out_request = usb.util.build_request_type(
    usb.util.CTRL_OUT
  , usb.util.CTRL_TYPE_VENDOR
  , usb.util.CTRL_RECIPIENT_DEVICE
)

DP_INFORMATION_IT = 0
DP_INFORMATION_IC = 1

DP_LED_TYPE_APA102 = 0
DP_LED_TYPE_WS2811 = 1

def get_status_ep1(device):
  status = device.ctrl_transfer(
      usb.util.build_request_type(usb.util.CTRL_IN, usb.util.CTRL_TYPE_STANDARD, usb.util.CTRL_RECIPIENT_ENDPOINT)
    , 0
    , 0
    , 1
    , 2
    , 100
  )
  print(status)


class DisplayController:
  "Object with USB display properties and some auxiliary functions."
  # Information types
  INFO_TYPE_IT_STATION = 0
  INFO_TYPE_IC_STRING = 1

  # LED types
  LED_TYPE_APA102 = 0
  LED_TYPE_WS2811 = 1

  def __init__(self, device):
    device.default_timeout = 500
    # work-around for xHCI behaviour
    # Set USB default configuration and then restore device configuration to force
    # endpoint data toggle reset in the host driver.
    device.set_configuration(0)
    device.set_configuration(1)
    self.device = device
    self.serial_number = device.serial_number
    self.__queryController()

  @staticmethod
  def __parseTlvData(data):
    offset = 0
    data = bytearray(data)
    while offset < len(data) and offset+1 < len(data):
      field_type = data[offset]
      field_length = data[offset+1]
      field_value = bytearray()
      offset += 2
      if field_length > 0:
        field_value = data[offset:offset+field_length]
        offset += field_length
      yield (field_type, field_length, field_value)

  def __queryController(self):
    DP_TYPE_INFORMATION_TYPE = 1
    DP_TYPE_INFORMATION_RANGE = 2
    DP_TYPE_LED_TYPE = 3
    DP_TYPE_BUFFER_SIZE = 4
    DP_TYPE_END = 0xff

    self.data_type = None
    self.data_ranges = list()
    self.led_type = None

    if self.device:
      data = self.device.ctrl_transfer(
          in_request # IN, vendor, device
        , 2 # Vendor request DISPLAY_PROPERTIES
        , 0
        , 0
        , (1<<8)
      )

      if struct.unpack("<H", data[:2])[0] != len(data):
        raise ValueError("Display information has invalid length")

      for t,l,v in self.__parseTlvData(data[2:]):
        if t == DP_TYPE_INFORMATION_TYPE:
          self.data_type = v[0]
        elif t == DP_TYPE_INFORMATION_RANGE:
          self.data_ranges.append((v[0], v[1]))
          self.data_ranges = sorted(self.data_ranges, key=lambda data_range: data_range[0])
        elif t == DP_TYPE_LED_TYPE:
          self.led_type = v[0]

  def transmitDisplayBuffer(self, data):
    try:
      # DEPRECATED
      # Write data via control command
#      self.device.ctrl_transfer(out_request, 1, 0, 0, data, 40)
      # NEW METHOD
      # Write data to EP1
      self.device.write(1, data, 40)
    except usb.core.USBError as usb_error:
      # TODO Error handling
      pass
#      if usb_error.errno == 32: # EP stall
#        try:
#          self.device.clear_halt(1)
#        except:
#          pass


class DisplayRange:
  def __init__(self, serial_number, range_type, start, end):
    self.serial_number = serial_number
    self.type = range_type
    self.start = start
    self.end = end

  def __lt__(self, other):
    # display range sorting:
    #   * Range type
    #   * Range start
    #   * Serial number
    if self.type != other.type:
      return self.type < other.type
    elif self.start != other.start:
      return self.start < other.start
    else:
      # Assume key is of form XX-YY-III-KKKK
      return int(self.serial_number.split('-')[:-1]) < int(other.serial_number.split('-')[:-1])

  def __repr__(self):
    return "({}:{}) for '{}'".format(self.start, self.end, self.serial_number)


class LogicalDisplay:
  def __init__(self, controllers):
    self.controllers = controllers

    # Determine unified display string range
    # Sets a dict of {controller : led_buffer_slice} items
    #   a dict of {string_number : led_buffer_offset} items
    self.__controller_buffer_slices = dict()
    self.__string_buffer_offset = dict()
    offset = 0
    offset_subbuffer = 0

    for controller in controllers:
      key = controller.serial_number
      controller_string_count = 0
      for data_range in controller.data_ranges:
        start,end = data_range
        controller_string_count += end-start+1
        for string in range(start,end+1):
          self.__string_buffer_offset[string] = offset
          offset += 1

      if controller.data_type == DisplayController.INFO_TYPE_IC_STRING:
        led_count = controller_string_count*60
      elif controller.data_type == DisplayController.INFO_TYPE_IT_STATION:
        led_count = controller_string_count
      else:
        raise ValueError("Unknown LED display type")

      self.__controller_buffer_slices[key] = slice(offset_subbuffer, offset_subbuffer+led_count)
      offset_subbuffer += led_count

    if self.string_count == 0:
      raise ValueError("Cannot create logical display with 0 detector strings")

  @property
  def string_count(self):
    return len(self.__string_buffer_offset)

  @property
  def data_type(self):
    return self.controllers[0].data_type

  def get_led_index(self, omkey):
    """Only provide om_key values for which canDisplayOMKey() returns True.
    Returns the buffer offset, modulo the LED data size.
    The first LED is at offset 0, second at offset 1, etc."""
    offset = self.__string_buffer_offset[omkey.string]
    if self.data_type == DisplayController.INFO_TYPE_IC_STRING:
        dom_offset = omkey.om - 1
        offset = 60*offset + dom_offset
    return offset

  @staticmethod
  def group_controllers(controllers):
    """
    Determine unified display string range
    Return a list of ((start, end), {controllers}) tuples
    """
    ranges = list()
    if isinstance(controllers, dict):
      for key,controller in controllers.items():
        for data_range in controller.data_ranges:
          ranges.append(DisplayRange(key, controller.data_type, *data_range))
    else:
      try:
        for controller in controllers:
          for data_range in controller.data_ranges:
            ranges.append(DisplayRange(controller.serial_number, controller.data_type, *data_range))
      except:
        raise ValueError("'controllers' is not iterable")

    # The following loop searches a (the last) range whose current end corresponds to the
    # start of the next segment. If multiple logical displays are present which can show
    # the same range, this will result in them being joined into multiple groups.
    # If multiple display groups exist for the same range, they will be grouped by serial number
    # FIXME If one range is longer than the other, the first range encountered by the loop will be
    #       the extended, irrespective of whether this is the correct serial number grouping
    groups = list()
    for display_range in sorted(ranges):
      segment_start = display_range.start
      segment_end = display_range.end
      segment_type = display_range.type
      segment_serial = display_range.serial_number
      i = len(groups)
      while i > 0:
        (start, end), group_controllers = groups[i-1]
        if end+1 == segment_start and display_range:
          end = segment_end
          group_controllers.add(segment_serial)
          groups[i-1] = ((start,end), group_controllers)
          break
        i -= 1

      if i == 0:
        groups.append( ((segment_start, segment_end), {segment_serial}) )

    return groups

# Controller discovery
# TODO Put in DisplayManager
controllers = dict()
for dev in usb.core.find(idVendor=0x1CE3, idProduct=2, find_all=True):
  controller = DisplayController(dev)
  controllers[dev.serial_number] = controller

groups = LogicalDisplay.group_controllers(controllers)
displays = list()
for group_range,group_controllers in groups:
  displays.append( LogicalDisplay([controllers[serial] for serial in group_controllers]) )

# end display management

string_count = 0
pixel_size = 3

if len(displays) > 0:
  display = displays[0]
  string_count = display.string_count
  print("Found display with {} strings".format(string_count))

  if len(sys.argv) > 1:
    offset = int(sys.argv[1])
  else:
    offset = 0

  while offset:
    buffer_size = string_count*pixel_size*60
    frame = bytearray(buffer_size)

    for subpixel in range(pixel_size):
      frame[(pixel_size*(offset%60) + subpixel)::pixel_size*60] = [0x10]*string_count

    frame = bytes(frame)

    class OmKey:
      def __init__(self, string, om):
        self.string = string
        self.om = om

    try:
      pass
#      written = device.write(1, frame, 40)
    except usb.core.USBError as e:
      print("Transfer {}".format(offset))
      if e.errno == 32: # EPIPE
        # Try to recover and resend
        device.clear_halt(1)
      raise e

    offset -= 1
    sleep(0.05)
