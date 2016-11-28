# -*- coding: utf-8 -*-
# Author: Sander Vanheule (Universiteit Gent) <sander.vanheule@ugent.be>
# Steamshovel module used to render and display IceCube data to an LED display connected via USB

__all__ = ["LedDisplay"]

# Initialise logging
from icecube.icetray import logging

try:
    import usb.core
    import usb.util
except:
    logging.log_notice("Failed to load USB support. LED displays are not supported.", "LedDisplay")
    raise ImportError

from icecube.shovelart import PyArtist
from icecube.shovelart import RangeSetting, ChoiceSetting, I3TimeColorMap
from icecube.shovelart import PyQColor, TimeWindowColor, StepFunctionFloat
from icecube.dataclasses import I3RecoPulseSeriesMapMask, I3RecoPulseSeriesMapUnion, I3OMGeo
import struct
import threading

class DisplayLed(object):
  "Class representing the color of an RGB LED with time dependent color and brightness."

  def __init__(self, brightness, color):
    """Create a new DisplayLed object with given `brightness` and `color` values.
    Both arguments may either be a function taking a single argument (time), or a static value.
    :param brightness: Float in range [0,1] or a function that returns such a value.
    :param color: PyQColor or a function that returns such a value."""
    if not hasattr(brightness, "__call__"):
      self._brightness = (lambda time : brightness)
    else:
      self._brightness = brightness

    if not hasattr(color, "__call__"):
      self._color = (lambda time : color)
    else:
      self._color = color

  @classmethod
  def float_to_led_data(cls, rgb):
    "Convert a 3-tuple of floats to the binary RGB format required by the supported LED."
    raise NotImplementedError

  def get_value(self, time):
    "Return a list of bytes containing the data provided to the LED."
    brightness = min(1.0, self._brightness(time)) # Clip brightness
    color = self._color(time)
    alpha = float(color.alpha)/255
    value = [comp*brightness*alpha for comp in color.rgbF()]
    return self.float_to_led_data(value)

class LedAPA102(DisplayLed):
  "APA102 data format: 5b global brightness, 3×8b RGB (4 bytes total)"
  DATA_LENGTH = 4
  MAX_BRIGHTNESS = 2**5-1

  @classmethod
  def float_to_led_data(cls, rgb):
    # Factor out brightness from colour
    brightness = max(1./cls.MAX_BRIGHTNESS, max(rgb))
    scaling = brightness / cls.MAX_BRIGHTNESS
    rgb = [int(round(255 * (c**2.2)/brightness)) for c in rgb]
    return [int(round(brightness*cls.MAX_BRIGHTNESS)), rgb[0], rgb[1], rgb[2]]


class LedWS2811(DisplayLed):
  "WS2811/WS2812 data format: 3×8b RGB"
  DATA_LENGTH = 3

  @classmethod
  def float_to_led_data(cls, rgb):
    return [int(round(255 * (c**2.2))) for c in rgb]


class DisplayController:
  __USB_VND_DEV_IN = usb.util.build_request_type(
    usb.util.CTRL_IN
  , usb.util.CTRL_TYPE_VENDOR
  , usb.util.CTRL_RECIPIENT_DEVICE
  )
  __USB_VND_DEV_OUT = usb.util.build_request_type(
      usb.util.CTRL_OUT
    , usb.util.CTRL_TYPE_VENDOR
    , usb.util.CTRL_RECIPIENT_DEVICE
  )

  "Object with USB display properties and some auxiliary functions."
  # Information types
  DATA_TYPE_IT_STATION = 0
  DATA_TYPE_IC_STRING = 1

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

  @property
  def buffer_length(self):
    pixel_length = 0
    if self.led_type == self.LED_TYPE_APA102:
      pixel_length = LedAPA102.DATA_LENGTH
    elif self.led_type == self.LED_TYPE_WS2811:
      pixel_length = LedWS2811.DATA_LENGTH

    string_length = 0
    if self.data_type == self.DATA_TYPE_IT_STATION:
      string_length = 1
    elif self.data_type == self.DATA_TYPE_IC_STRING:
      string_length = 60

    string_count = 0
    for (start, end) in self.data_ranges:
      string_count += end - (start-1)

    return pixel_length*string_length*string_count

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
          self.__USB_VND_DEV_IN
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
      # Write data to EP1
      self.device.write(1, data, 40)
    except usb.core.USBError as usb_error:
      # TODO Better error handling
      print(usb_error.errno)
      if usb_error.errno == 32:
        # EP stalled, try to clear
        try:
          self.device.clear_halt(1)
        except:
          pass
      elif usb_error.errno == 19:
        # No such device. Maybe it was reattached to the USB port, so try to find it back
        try:
          for device in usb.core.find(idVendor=0x1CE3, find_all=True):
            if device.serial_number == self.serial_number:
              self.device = device
        except:
          pass


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


class DisplayWorker(threading.Thread):
  "Helper thread to transmit frame data to a single USB device."

  def __init__(self, controller, buffer):
    super(DisplayWorker, self).__init__()
    self.__buffer = buffer
    self.__controller = controller

  def run(self):
    if self.__buffer is not None:
      self.__controller.transmitDisplayBuffer(self.__buffer)


class LogicalDisplay:
  def __init__(self, controllers, multithreading=False):
    self.__multithreading = multithreading
    # \a controllers should be a list of controllers that displays a continuous string range
    if len(controllers) == 0:
      raise ValueError("Cannot create a display with 0 controllers")

    # Determine unified display string range
    # Sets a dict of {controller : led_buffer_slice} items
    #   a dict of {string_number : led_buffer_offset} items
    self.__buffer_slices = dict()
    self.__string_buffer_offset = dict()
    self.__buffer_length = 0
    offset = 0

    self.controllers = dict()
    self.__data_type = None
    led_type = None
    self.__range_start = None
    self.__range_end = None
    for controller in controllers:
      key = controller.serial_number
      self.controllers[key] = controller

      if self.__data_type is None:
        self.__data_type = controller.data_type
        led_type = controller.led_type
      elif self.__data_type != controller.data_type:
        raise ValueError("Cannot create a single display with multiple data types")
      elif led_type != controller.led_type:
        raise ValueError("Cannot create a single display with multiple LED types")


      controller_string_count = 0
      for data_range in controller.data_ranges:
        start, end = data_range
        # Determine display string range
        if self.__range_start is None:
          self.__range_start = start
          self.__range_end = end
        else:
          self.__range_start = min(self.__range_start, start)
          self.__range_end = max(self.__range_end, end)
        # Calculate string to buffer offset mapping
        for string in range(start,end+1):
          self.__string_buffer_offset[string] = offset
          offset += 1

      self.__buffer_slices[key] = slice(
          self.__buffer_length
        , self.__buffer_length + controller.buffer_length
      )
      self.__buffer_length += controller.buffer_length

    if self.__buffer_length == 0:
      raise ValueError("Cannot create logical display with 0 buffer length")

    # If control gets here, the internal state should be valid
    if led_type == DisplayController.LED_TYPE_APA102:
      self.__led_class = LedAPA102
    elif led_type == DisplayController.LED_TYPE_WS2811:
      self.__led_class = LedWS2811
    else:
      raise ValueError("Unknown LED type: {}".format(led_type))
      self.__led_class = None

  @property
  def string_count(self):
    return len(self.__string_buffer_offset)

  @property
  def string_range(self):
    return (self.__range_start, self.__range_end)

  @property
  def data_type(self):
    return self.__data_type

  @property
  def led_class(self):
    return self.__led_class

  @property
  def buffer_length(self):
    return self.__buffer_length

  def canDisplayOMKey(self, geometry, om_key):
    """Check wether the display can display a certain DOM."""
    valid_dom = False
    omgeo = geometry.omgeo
    if self.__data_type == DisplayController.DATA_TYPE_IC_STRING:
        valid_dom = om_key in omgeo and omgeo[om_key].omtype == I3OMGeo.IceCube
    elif self.__data_type == DisplayController.DATA_TYPE_IT_STATION:
        valid_dom = om_key in omgeo and omgeo[om_key].omtype == I3OMGeo.IceTop
    return valid_dom and (om_key.string in self.__string_buffer_offset)

  def getLedIndex(self, om_key):
    """Only provide om_key values for which canDisplayOMKey() returns True.
    Returns the buffer offset, modulo the LED data size.
    The first LED is at offset 0, second at offset 1, etc."""
    offset = self.__string_buffer_offset[om_key.string]
    if self.data_type == DisplayController.DATA_TYPE_IC_STRING:
        dom_offset = om_key.om - 1
        offset = 60*offset + dom_offset
    return offset

  def transmitDisplayBuffer(self, data):
    if len(data) != self.__buffer_length:
      raise ValueError("Data buffer has invalid length")

    if not self.__multithreading:
      # Single threaded working code
      for key in self.controllers.keys():
        self.controllers[key].transmitDisplayBuffer(data[self.__buffer_slices[key]])
    else:
      # Multi process code
      workers = list()
      for key in self.controllers.keys():
        w = DisplayWorker(self.controllers[key], data[self.__buffer_slices[key]])
        w.start()
        workers.append(w)

      for w in workers:
        w.join()

  def close(self):
    # Placeholder function
    pass


class DisplayManager:
  def __init__(self):
    controllers = dict()
    for dev in usb.core.find(idVendor=0x1CE3, find_all=True):
      controller = DisplayController(dev)
      controllers[dev.serial_number] = controller

    groups = self.__groupControllers(controllers)

    self.__displays = list()
    for group_range,group_controllers in groups:
      display = LogicalDisplay([controllers[serial] for serial in group_controllers], False)
      self.__displays.append(display)
      # Log device TODO
#      logging.log_info(
#           "Found [usb:{bus:03d}-{address:03d}] {product} for {range_type} {ranges} (SN: {serial})".format(**info)
#         , "LedDisplay"
#      )

  @property
  def displays(self):
    return self.__displays

  @staticmethod
  def __groupControllers(controllers):
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


class NewLedDisplay(PyArtist):
    numRequiredKeys = 2
    _SETTING_DEVICE = "Device"
    _SETTING_COLOR_STATIC = "Static color"
    _SETTING_BRIGHTNESS_STATIC = "Static brightness"
    _SETTING_COLOR = "Colormap"
    _SETTING_INFINITE_DURATION = "Use infinite pulses"
    _SETTING_DURATION = "Finite pulse duration (log10 ns)"
    _SETTING_COMPRESSION_POWER = "Compression"

    _manager = DisplayManager()

    def __init__(self):
        PyArtist.__init__(self)

        self._leds = {}

        self._display = None

        device_descriptions = []
        if len(self._manager.displays) > 0:
            for display in self._manager.displays:
                # Collect device info
                info = {
                      "start" : display.string_range[0]
                    , "end" : display.string_range[1]
                    , "range_type" : "range"
                    , "product" : "LED display"
                }
                if display.data_type == DisplayController.DATA_TYPE_IC_STRING:
                    info["product"] = "IceCube display"
                    info["range_type"] = "strings"
                elif display.data_type == DisplayController.DATA_TYPE_IT_STATION:
                    info["product"] = "IceTop display"
                    info["range_type"] = "stations"
                # Add to list of descriptors
                description = "{product} ({range_type} {start}-{end})".format(**info)
                device_descriptions.append(description)
        else:
            device_descriptions.append("no devices detected")

        self.addSetting(self._SETTING_DEVICE, ChoiceSetting(device_descriptions, 0))
        self.addSetting(self._SETTING_COLOR_STATIC, PyQColor.fromRgb(255, 0, 255))
        self.addSetting(self._SETTING_BRIGHTNESS_STATIC, RangeSetting(0.0, 1.0, 32, 0.5))
        self.addSetting(self._SETTING_COMPRESSION_POWER, RangeSetting(0.0, 1.0, 40, 0.18))
        self.addSetting(self._SETTING_COLOR, I3TimeColorMap())
        self.addSetting(self._SETTING_INFINITE_DURATION, True)
        self.addSetting(self._SETTING_DURATION, RangeSetting(1.0, 6.0, 40, 5.0))
        self.addCleanupAction(self._cleanupDisplay)

    def _connectToDisplay(self, display):
        # Close any old connection
        if self._display:
            self._display.close()
        # New connection
        if display:
            self._display = display
        else:
            self._display = None

    def _cleanupDisplay(self):
        if self._display:
            logging.log_debug("Clearing display", "LedDisplay")
            # Blank display and release USB device interface
            self._display.transmitDisplayBuffer(bytes(bytearray(self._display.buffer_length)))
            self._display.close()

    def description(self):
        return "LED event display driver"

    def isValidKey(self, frame, key_idx, key):
        key_type = frame.type_name(key)
        logging.log_trace(
            "key '{}' of type '{}' offered for key index {}".format(key, key_type, key_idx)
          , "LedDisplay"
        )
        if key_idx == 0:
            return "I3Geometry" == key_type
        elif key_idx == 1:
            # Key may be any container of OMKeys or a mask
            # copied from Bubbles.py
            return (   ("OMKey" in key_type and not "I3ParticleID" in key_type )
                    or key_type.endswith("MapMask")
                    or key_type.endswith("MapUnion"))
        else:
            return False

    @staticmethod
    def _merge_lists(left, right, key=lambda x: x):
        "Merge two already sorted lists into a single sorted list."
        merged = []

        # Pick smallest from top while both lists have items
        i = 0
        j = 0
        while (i < len(left) and j < len(right)):
            if key(left[i]) < key(right[j]):
                merged.append(left[i])
                i = i+1
            else:
                merged.append(right[j])
                j = j+1

        # Add remaining items from left, or right
        # Note that both cases are mutually exclusive
        if i < len(left):
            merged.extend(left[i:])
        if j < len(right):
            merged.extend(right[j:])

        return merged

    def _handleOMKeyMapTimed(self, output, geometry, omkey_pulses_map):
        """Parse a map of OMKey to pulse series.
        :returns: A dict of LED buffer offsets to LedDisplay objects."""
        color_map = self.setting(self._SETTING_COLOR)

        if self.setting(self._SETTING_INFINITE_DURATION):
            duration = None
        else:
            duration = 10**self.setting(self._SETTING_DURATION)

        # content of led_pulses: {led : [(time, charge-like)]} {int : [(float, float)]}
        led_pulses = {}

        for omkey, pulses in omkey_pulses_map:
            logging.log_trace(
                "Data available for DOM {}-{}".format(omkey.string, omkey.om)
              , "LedDisplay"
            )
            if self._display.canDisplayOMKey(geometry, omkey):
                led = self._display.getLedIndex(omkey)
                logging.log_trace("Placing data at buffer index {}".format(led), "LedDisplay")
                # Ensure we're dealing with a list of pulses
                if not hasattr(pulses, "__len__"):
                    pulses = list(pulses)

                if len(pulses) > 0:
                    if led not in led_pulses:
                        led_pulses[led] = pulses
                    else:
                        # In case multiple DOMs are mapped onto the same LED, merge the pulses
                        # Merge two already sorted lists (merge sort)
                        led_pulses[led] = self._merge_lists(
                              led_pulses[led]
                            , pulses
                            , key=lambda pulse : pulse.time
                        )

        # Determine event normalisation
        max_sum_charges = 0.
        charges = {}
        for led in led_pulses:
            pulses = led_pulses[led]
            t0 = pulses[0].time
            has_npe = hasattr(pulses[0], "npe")
            has_charge = hasattr(pulses[0], "charge")

            total_charge = 0.
            if has_charge:
                charges[led] = [(pulse.time, pulse.charge) for pulse in pulses]
                total_charge += pulse.charge
            elif has_npe:
                charges[led] = [(pulse.time, pulse.npe) for pulse in pulses]
                total_charge += pulse.npe
            else:
                charges[led] = [(pulse.time, 1.0) for pulse in pulses]
                total_charge += 1.0

            if total_charge > max_sum_charges:
                max_sum_charges = total_charge

        # Iterate second time for light curves
        led_curves = {}
        normalisation = max_sum_charges
        power = self.setting(self._SETTING_COMPRESSION_POWER)
        for led in charges:
            brightness = StepFunctionFloat(0)
            pulses = charges[led]
            t0 = pulses[0][0]

            if duration:
                tail = 0
                head = None
                # Determine pulse intervals
                t0s = [t0]
                while tail < len(pulses):
                    accumulated_charge = 0.0
                    head = tail
                    t, q = pulses[head]
                    # Add accumulated charge of currently visible pulses
                    while pulses[head][0]+duration > t and head >= 0:
                        accumulated_charge += pulses[head][1]
                        head -= 1
                    brightness.add((accumulated_charge/normalisation)**power, t)
                    # If the current sequence doesn't overlap with the next pulse, reset the
                    # brightness and register the new series starting point
                    if tail < len(pulses)-1 and t+duration < pulses[tail+1][0]:
                        t0s.append(pulses[tail+1][0])
                        brightness.add(0, t+duration)
                    tail += 1
                # Now `tail == len(pulses)`, but the brightness curve is still at the last
                # accumulated charge
                # If `head` is 0, its interval is most likely still included for the total
                # charge, so progress to the next change point
                if pulses[head][0]+duration <= pulses[-1][0]:
                    head += 1
                # Scan the ends of the display intervals to see what charge is still remaining
                while head < len(pulses):
                    t, q = pulses[head]
                    accumulated_charge = 0.0
                    tail = head + 1
                    while tail < len(pulses):
                        accumulated_charge += pulses[tail][1]/normalisation
                        tail += 1
                    brightness.add(accumulated_charge**power, t+duration)
                    head += 1

                color = TimeWindowColor(output, t0s, color_map)
            else:
                accumulated_charge = 0.0
                color = TimeWindowColor(output, t0, color_map)
                for t, q in pulses:
                    accumulated_charge += q/normalisation
                    brightness.add(accumulated_charge**power, t)

            led_curves[led] = self._display.led_class(brightness.value, color.value)

        return led_curves

    def _handleOMKeyListStatic(self, output, geometry, omkey_list):
        """Parse a map of OMKey to static values.
        :returns: A dict of LED buffer offsets to LedDisplay objects."""
        color_static = self.setting(self._SETTING_COLOR_STATIC)
        brightness_static = self.setting(self._SETTING_BRIGHTNESS_STATIC)

        led_curves = {}

        for omkey in omkey_list:
            logging.log_trace(
                "Data available for DOM {}-{}".format(omkey.string, omkey.om)
              , "LedDisplay"
            )
            if self._display.canDisplayOMKey(geometry, omkey):
                led = self._display.getLedIndex(omkey)
                if led not in led_curves:
                    led_curves[led] = self._display.led_class(
                          brightness_static
                        , color_static
                    )

        return led_curves

    def create(self, frame, output):
        # Connect to newly selected display, if any
        display_index = self.setting(self._SETTING_DEVICE)
        if display_index < len(self._manager.displays):
          new_display = self._manager.displays[display_index]
        else:
          new_display = None

        if self._display != new_display:
            self._display = new_display

        if self._display:
            (geom_key, data_key) = self.keys()
            geometry = frame[geom_key]
            omkey_object = frame[data_key]

            # If we are displaying a mask/union, ensure omkey_object is a I3RecoPulseSeries object
            if isinstance(omkey_object, (I3RecoPulseSeriesMapMask, I3RecoPulseSeriesMapUnion)):
                omkey_object = omkey_object.apply(frame)

            if hasattr(omkey_object, "keys"): # I3Map of OMKeys
                if len(omkey_object) > 0:
                    first_value = omkey_object[omkey_object.keys()[0]]
                    if hasattr(first_value, "__len__") and len(first_value) > 0:
                        if hasattr(first_value[0], "time"):
                            self._leds = self._handleOMKeyMapTimed(output, geometry, omkey_object)
                    elif hasattr(first_value, "time"):
                        self._leds = self._handleOMKeyMapTimed(output, geometry, omkey_object)
                    else:
                        self._leds = self._handleOMKeyListStatic(output, omkey_object.keys())
            elif hasattr(omkey_object, "__len__"):
                if len(omkey_object) > 0:
                    self._leds = self._handleOMKeyListStatic(output, omkey_object)

            output.addPhantom(self._cleanupEvent, self._updateEvent)

    def _updateEvent(self, event_time):
        if self._display:
            data_length = self._display.led_class.DATA_LENGTH
            frame = bytearray(self._display.buffer_length)

            for led in self._leds.keys():
                led_value = self._leds[led].get_value(event_time)
                frame[led*data_length:(led+1)*data_length] = led_value

            self._display.transmitDisplayBuffer(bytes(frame))

    def _cleanupEvent(self):
        self._leds = {}
