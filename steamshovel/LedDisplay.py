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
from icecube.dataclasses import I3RecoPulseSeriesMapMask, I3RecoPulseSeriesMapUnion
import struct

def merge_lists(left, right, key=lambda x: x):
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

    def getValue(self, time):
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


class TlvParser(object):
    """Parser to interpret binary TLV data provided by a USB display's properties report.
    TLV data consists of a series of (1 type byte, 1 length byte (value N), and N data bytes)."""
    # TLV types
    DP_INFORMATION_TYPE = 1
    DP_INFORMATION_RANGE = 2
    DP_LED_TYPE = 3

    @staticmethod
    def _tokenizeData(data):
        """This function accepts a byte buffer and returns a list of bytearray objects, each item
        in the list containing one TLV field."""
        tokens = []

        i = 0
        data = bytearray(data)
        while i+1 < len(data):
            field_type = data[i]
            field_length = data[i+1]
            field_value = bytearray()
            i += 2
            if field_length > 0:
                field_value = data[i:i+field_length]
                i += field_length
            tokens.append((field_type, field_length, field_value))

        return tokens

    @classmethod
    def _parseTokens(cls, display_properties, token_list):
        """Takes a list of TLV fields (`token_list`) and stores the interpreted data in the
        `display_properties` object."""
        for token in token_list:
            t, l, v = token
            if t == cls.DP_INFORMATION_TYPE and l == 1:
                display_properties.info_type = v[0]
            elif t == cls.DP_INFORMATION_RANGE and l == 2:
                display_properties.addInformationRange(v[0], v[1])
            elif t == cls.DP_LED_TYPE and l == 1:
                display_properties.setLedType(v[0])

    @classmethod
    def parseData(cls, display_properties, data):
        """Take a binary display properties report and store the contained information in
        `display_properties."""
        return cls._parseTokens(display_properties, cls._tokenizeData(data))


class UsbDisplayProperties(object):
    "Object with USB display properties and some auxiliary functions."
    # Information types
    INFO_TYPE_IT_STATION = 0
    INFO_TYPE_IC_STRING = 1

    # LED types
    LED_TYPE_APA102 = 0
    LED_TYPE_WS2811 = 1

    def __init__(self, usb_device):
        self.usb_device = usb_device
        self.info_type = None
        self.info_ranges = list()
        self.led_class = None
        self._strings = list()
        self._string_index = dict()

    def setLedType(self, led_type):
        """Set the display's LED type. UsbDisplayProperties will be set to contain the
        corresponding DisplayLed class."""
        if led_type == self.LED_TYPE_APA102:
            self.led_class = LedAPA102
        elif led_type == self.LED_TYPE_WS2811:
            self.led_class = LedWS2811

    @property
    def strings(self):
        return self._strings

    @property
    def string_index_map(self):
        return self._string_index

    def addInformationRange(self, start, end):
        """Add a range to the currently supported information ranges.
        `start` and `end` are inclusive."""
        # Python ranges are endpoint-exclusive: [start, end)
        # Ours are endpoint-inclusive: [start,end]
        self.info_ranges.append((start,end))
        self._strings = merge_lists(self._strings, range(start, end+1))
        self._string_index = {string: index for (index, string) in enumerate(self._strings)}

    def canDisplayOMKey(self, om_key):
        """Check wether the display can display a certain DOM. Requires UsbDisplayProperties to be
        set to a valid value. Otherwise this function will always return False."""
        valid_dom = False
        if self.info_type == self.INFO_TYPE_IC_STRING:
            valid_dom = (om_key.om >= 1) and (om_key.om <= 60)
        elif self.info_type == self.INFO_TYPE_IT_STATION:
            valid_dom = (om_key >= 60) and (om_key.om <= 64)
        return valid_dom and (om_key.string in self._strings)

    def getFrameSize(self):
        "Return the size of a display frame in bytes."
        strings = len(self._strings)
        bytes_per_led = self.led_class.DATA_LENGTH
        if self.info_type == self.INFO_TYPE_IC_STRING:
            return strings*60*bytes_per_led
        elif self.info_type == self.INFO_TYPE_IT_STATION:
            return strings*bytes_per_led
        else:
            return 0

    def getLedIndex(self, om_key):
        """Only provide om_key values for which canDisplayOMKey() returns True.
        Returns the buffer offset, modulo the LED data size.
        The first LED is at offset 0, second at offset 1, etc."""
        offset = self._string_index[om_key.string]
        if self.info_type == self.INFO_TYPE_IC_STRING:
            dom_offset = om_key.om - 1
            offset = 60*offset + dom_offset
        return offset


class DisplayConnection(object):
    "Handle USB connection to display"

    # USB control transfers
    REQUEST_SEND_FRAME = 1
    REQUEST_DISPLAY_PROPERTIES = 2
    REQUEST_EEPROM_WRITE = 3
    REQUEST_EEPROM_READ = 4

    _REQ_IN = usb.util.build_request_type(
          usb.util.CTRL_IN
        , usb.util.CTRL_TYPE_VENDOR
        , usb.util.CTRL_RECIPIENT_DEVICE
    )
    _REQ_OUT = usb.util.build_request_type(
          usb.util.CTRL_OUT
        , usb.util.CTRL_TYPE_VENDOR
        , usb.util.CTRL_RECIPIENT_DEVICE
    )

    def __init__(self):
        # Public field
        self.device = None
        # Private connection
        self._usb_interface = None
        # LED display properties
        self._led_count = 0
        self._led_class = None

    def isConnected(self):
        "Check if there is an active connection."
        return (self.device is not None) and (self._usb_interface is not None)

    @classmethod
    def enumerateDevices(cls):
        "Get a list of available devices. Returns a list of `UsbDisplayProperties` objects."
        usb_devices = []

        for device in usb.core.find(idVendor=0x1CE3, find_all=True):
            if device.idProduct == 1 or device.idProduct == 2:
                display = cls.queryDevice(device)
                if display:
                    usb_devices.append(display)

        return usb_devices

    def connectDevice(self, device):
        """Try to connect to a USB device.
        :param usb.core.Device device: Device to connect with."""
        # Release current device before attempting new connection
        self.close()
        if not self.device and device:
            bus = device.bus
            address = device.address
            dev_name = "[usb:{:03d}-{:03d}]".format(bus, address)
            try:
                self.device = device
                self.device.set_configuration(1)
                self._usb_interface = self.device.get_active_configuration().interfaces()[0]
                usb.util.claim_interface(self.device, self._usb_interface)
                logging.log_debug("Opened device {}".format(dev_name), "LedDisplay")
            except Exception as e:
                self.device = None
                self._usb_interface = None
                logging.log_warn("Could not open device {}".format(dev_name), "LedDisplay")
        # Return if connection was succesful
        return self._usb_interface is not None

    def close(self):
        "Close current USB connection, if any."
        if self.device and self._usb_interface:
            try:
                usb.util.release_interface(self.device, self._usb_interface)
            except:
                pass
            finally:
                self.device = None
                self._usb_interface = None
                logging.log_debug("Released USB display interface", "LedDisplay")

    @classmethod
    def queryDevice(cls, device):
        """Get display properties from the given device.
        :param usb.core.Device device: Device to interrogate.
        :return: A DisplayProperties object or None if device was not available.
        """
        properties = None
        if device:
            VENDOR_IN = cls._REQ_IN
            # Read only properties header: properties length
            data = device.ctrl_transfer(cls._REQ_IN, cls.REQUEST_DISPLAY_PROPERTIES, 0, 0, 2, 5000)
            size = struct.unpack('<H', data)[0]
            # Read full TLV list, skip header this time
            data = device.ctrl_transfer(
                  cls._REQ_IN
                , cls.REQUEST_DISPLAY_PROPERTIES
                , 0
                , 0
                , size
                , 5000
            )
            properties = UsbDisplayProperties(device)
            TlvParser.parseData(properties, data[2:])
        return properties

    def writeFrame(self, frame):
        """Write a full frame to the device using a control request.
        :param bytes frame: Frame data to be displayed."""
        if self.device:
            try:
                self.device.ctrl_transfer(
                      self._REQ_OUT
                    , self.REQUEST_SEND_FRAME
                    , 0
                    , 0
                    , frame
                    , 5000
                )
            except Exception as e:
                logging.log_error("Could not write frame to display: {}".format(e))
                self.close()


class LedDisplay(PyArtist):
    numRequiredKeys = 1
    _SETTING_DEVICE = "Device"
    _SETTING_COLOR_STATIC = "Static color"
    _SETTING_BRIGHTNESS_STATIC = "Static brightness"
    _SETTING_COLOR = "Colormap"
    _SETTING_INFINITE_DURATION = "Use infinite pulses"
    _SETTING_DURATION = "Finite pulse duration (log10 ns)"
    _SETTING_COMPRESSION_POWER = "Compression"

    def __init__(self):
        PyArtist.__init__(self)

        self._leds = {}

        self._usb_displays = DisplayConnection.enumerateDevices()
        self._current_display = None
        self._connection = DisplayConnection()
        device_descriptions = []
        if len(self._usb_displays) > 0:
            for display in self._usb_displays:
                # Collect device info
                info = {
                      "bus": display.usb_device.bus
                    , "address": display.usb_device.address
                    , "product": display.usb_device.product
                    , "serial": display.usb_device.serial_number
                    , "range_type": "ranges"
                    , "ranges": ", ".join(["{}-{}".format(s,e) for (s,e) in display.info_ranges])
                }
                if display.info_type == UsbDisplayProperties.INFO_TYPE_IC_STRING:
                    info["range_type"] = "strings"
                elif display.info_type == UsbDisplayProperties.INFO_TYPE_IT_STATION:
                    info["range_type"] = "stations"
                # Log device
                logging.log_info(
                     "Found [usb:{bus:03d}-{address:03d}] {product} for {range_type} {ranges} (SN: {serial})".format(**info)
                   , "LedDisplay"
                )
                # Add to list of descriptors
                description = "{product} ({range_type} {ranges})".format(**info)
                device_descriptions.append(description)
        else:
            self._usb_displays.append(None)
            device_descriptions.append("no devices detected")

        self.addSetting(self._SETTING_DEVICE, ChoiceSetting(device_descriptions, 0))
        self.addSetting(self._SETTING_COLOR_STATIC, PyQColor.fromRgb(255, 0, 255))
        self.addSetting(self._SETTING_BRIGHTNESS_STATIC, RangeSetting(0.0, 1.0, 32, 0.5))
        self.addSetting(self._SETTING_COMPRESSION_POWER, RangeSetting(0.0, 1.0, 40, 0.18))
        self.addSetting(self._SETTING_COLOR, I3TimeColorMap())
        self.addSetting(self._SETTING_INFINITE_DURATION, True)
        self.addSetting(self._SETTING_DURATION, RangeSetting(1.0, 5.0, 40, 5.0))
        self.addCleanupAction(self._cleanupDisplay)

    def _connectToDisplay(self, display):
        # Close any old connection
        if self._connection.isConnected():
            self._connection.close()
        # New connection
        if display and self._connection.connectDevice(display.usb_device):
            self._current_display = display
        else:
            self._current_display = None

    def _cleanupDisplay(self):
        if self._connection.isConnected():
            logging.log_debug("Clearing display", "LedDisplay")
            # Blank display and release USB device interface
            self._connection.writeFrame(bytes(bytearray(self._current_display.getFrameSize())))
            self._connection.close()

    def description(self):
        return "IceTop LED event display driver"

    def isValidKey(self, frame, key_idx, key):
        key_type = frame.type_name(key)
        logging.log_trace(
            "key '{}' of type '{}' offered for key index {}".format(key, key_type, key_idx)
          , "LedDisplay"
        )
        if key_idx == 0:
            # Key may be any container of OMKeys or a mask
            # copied from Bubbles.py
            return (   ("OMKey" in key_type and not "I3ParticleID" in key_type )
                    or key_type.endswith("MapMask")
                    or key_type.endswith("MapUnion"))
        else:
            return False

    def _handleOMKeyMapTimed(self, output, omkey_pulses_map):
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
            if self._current_display.canDisplayOMKey(omkey):
                led = self._current_display.getLedIndex(omkey)
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
                        led_pulses[led] = merge_lists(
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

            led_curves[led] = self._current_display.led_class(brightness.value, color.value)

        return led_curves

    def _handleOMKeyListStatic(self, output, omkey_list):
        """Parse a map of OMKey to static values.
        :returns: A dict of LED buffer offsets to LedDisplay objects."""
        color_static = self.setting(self._SETTING_COLOR_STATIC)
        brightness_static = self.setting(self._SETTING_BRIGHTNESS_STATIC)
        display = self._current_display

        led_curves = {}

        for omkey in omkey_list:
            logging.log_trace(
                "Data available for DOM {}-{}".format(omkey.string, omkey.om)
              , "LedDisplay"
            )
            if self._current_display.canDisplayOMKey(omkey):
                led = self._current_display.getLedIndex(omkey)
                if led not in led_curves:
                    led_curves[led] = self._current_display.led_class(
                          brightness_static
                        , color_static
                    )

        return led_curves

    def create(self, frame, output):
        # Connect to newly selected display, if any
        new_display = self._usb_displays[self.setting(self._SETTING_DEVICE)]
        if self._current_display != new_display:
            self._connectToDisplay(new_display)

        if self._current_display:
            (frame_key,) = self.keys()
            omkey_object = frame[frame_key]

            # If we are displaying a mask/union, ensure omkey_object is a I3RecoPulseSeries object
            if isinstance(omkey_object, (I3RecoPulseSeriesMapMask, I3RecoPulseSeriesMapUnion)):
                omkey_object = omkey_object.apply(frame)

            if hasattr(omkey_object, "keys"): # I3Map of OMKeys
                if len(omkey_object) > 0:
                    first_value = omkey_object[omkey_object.keys()[0]]
                    if hasattr(first_value, "__len__") and len(first_value) > 0:
                        if hasattr(first_value[0], "time"):
                            self._leds = self._handleOMKeyMapTimed(output, omkey_object)
                    elif hasattr(first_value, "time"):
                        self._leds = self._handleOMKeyMapTimed(output, omkey_object)
                    else:
                        self._leds = self._handleOMKeyListStatic(output, omkey_object.keys())
            elif hasattr(omkey_object, "__len__"):
                if len(omkey_object) > 0:
                    self._leds = self._handleOMKeyListStatic(output, omkey_object)

            output.addPhantom(self._cleanupEvent, self._updateEvent)

    def _updateEvent(self, event_time):
        if self._connection.isConnected():
            data_length = self._current_display.led_class.DATA_LENGTH
            frame = bytearray(self._current_display.getFrameSize())

            for led in self._leds:
                led_value = self._leds[led].getValue(event_time)
                frame[led*data_length:(led+1)*data_length] = led_value

            self._connection.writeFrame(bytes(frame))

    def _cleanupEvent(self):
        self._leds = {}
