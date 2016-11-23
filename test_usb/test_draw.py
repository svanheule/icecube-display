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

    self.display_type = None
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
          self.display_type = v[0]
        elif t == DP_TYPE_INFORMATION_RANGE:
          self.data_ranges.append((v[0], v[1]))
          self.data_ranges = sorted(self.data_ranges, key=lambda data_range: data_range[0])
        elif t == DP_TYPE_LED_TYPE:
          self.led_type = v[0]

joined_ranges = list()
controller_buffer_slices = dict()
string_buffer_offset = dict()

def join_controller_ranges(controllers):
  """
  Determine unified display string range
  Sets a list of ((start, end), {controllers}) tuples
       a dict of {controller : buffer_slice} items
       a dict of {string : buffer_offset} items
  """
  segments = list()
  global string_buffer_offset
  offset = 0
  offset_subbuffer = 0

  for key,controller in controllers.items():
    string_count = 0
    for data_range in controller.data_ranges:
      segments.append((key, data_range))
      start,end = data_range
      string_count += end-start+1
      for string in range(start,end+1):
        string_buffer_offset[string] = offset
        offset += 1

    controller_buffer_slices[key] = slice(offset_subbuffer, offset_subbuffer+string_count*60)
    offset_subbuffer += string_count*60

  segments = sorted(segments, key=lambda seg: seg[1][0])

  joined = list()

  for controller,segment_range in segments:
    segment_start, segment_end = segment_range
    i = len(joined)
    while i > 0:
      (start,end),controllers = joined[i-1]
      if end+1 == segment_start:
        end = segment_end
        controllers.add(controller)
        joined[i-1] = ((start,end), controllers)
        break
      i -= 1

    if i == 0:
      joined.append( ((segment_start, segment_end), {controller}) )

  global joined_ranges
  joined_ranges = joined

controllers = dict()
for dev in usb.core.find(idVendor=0x1CE3, idProduct=2, find_all=True):
  controller = DisplayController(dev)
  controllers[dev.serial_number] = controller

join_controller_ranges(controllers)
string_count = 0
pixel_size = 3

#print(string_buffer_offset)
#print({key :3*(s.stop-s.start) for (key,s) in controller_buffer_slices.items()})

if len(joined_ranges) == 1:
  (start,end),controllers = joined_ranges[0]
  string_count = (end-start+1)

if string_count > 0:
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
