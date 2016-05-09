import logging
import time

# Initialise logging
_log = logging.getLogger("shovelart.leddisplay")
_log.setLevel(logging.INFO)
_log_handler = logging.FileHandler("/tmp/shovelart-leddisplay.log", mode='w')
_log_handler.setLevel(logging.INFO)
_log_format = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
_log_handler.setFormatter(_log_format)
_log.addHandler(_log_handler)

_log.info("leddisplay loaded")

try:
  import sys
  # libusb1 support
  from usb1 import USBContext, ENDPOINT_OUT, TYPE_VENDOR, RECIPIENT_INTERFACE
except:
  USBContext = None
  _log.error("Failed to import python-libusb1")

from icecube.shovelart import PyArtist
from icecube.shovelart import RangeSetting, ChoiceSetting, I3TimeColorMap
from icecube.shovelart import PyQColor, TimeWindowColor, StepFunctionFloat
from icecube.dataclasses import I3RecoPulseSeriesMapMask, I3RecoPulseSeriesMapUnion

def merge_lists(left, right, key=lambda x: x):
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

class TimeWindowColorSequence(object):
    def __init__(self, output, times, colormap):
        self._windows = [(t, TimeWindowColor(output, t, colormap)) for t in times]

    def value(self, time):
        i = 0
        while i < len(self._windows) and self._windows[i][0] < time:
            i += 1

        if i > 0:
            return self._windows[i-1][1].value(time)
        elif len(self._windows) > 0: # i == 0 and window list is not empty
            return self._windows[0][1].value(time)
        else:
            return PyQColor.fromRgb(0, 0, 0)


class StationLed(object):
    MAX_BRIGHTNESS = 2**5-1
    DATA_LENGTH = 4

    def __init__(self, brightness, color):
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
        "APA102 data format: 5b global brightness, 3*8b RGB"
        # Factor out brightness from colour
        brightness = max(1./cls.MAX_BRIGHTNESS, max(rgb))
        scaling = brightness / cls.MAX_BRIGHTNESS
        rgb = [int(round(255 * (c**2.2)/brightness)) for c in rgb]
        return [int(round(brightness*cls.MAX_BRIGHTNESS)), rgb[0], rgb[1], rgb[2]]

    def getValue(self, time):
        brightness = min(1.0, self._brightness(time)) # Clip brightness
        color = self._color(time)
        alpha = float(color.alpha)/255
        value = [comp*brightness*alpha for comp in color.rgbF()]
        return self.float_to_led_data(value)


class LedDisplay(PyArtist):
    _LED_COUNT = 78
    _FRAME_SIZE = _LED_COUNT*StationLed.DATA_LENGTH

    numRequiredKeys = 1
    _SETTING_DEVICE = "device"
    _SETTING_COLOR_STATIC = "static_color"
    _SETTING_BRIGHTNESS_STATIC = "static_brightness"
    _SETTING_COLOR = "colormap"
    _SETTING_INFINITE_DURATION = "infinite_pulses"
    _SETTING_DURATION = "duration"
    _SETTING_COMPRESSION_POWER = "compression"

    def __init__(self):
        PyArtist.__init__(self)

        # USB Device handle field
        self._usb_handle = None
        self._stations = {}

        self._usb_devices = []
        usb_device_descriptions = []
        if USBContext:
            usb_context = USBContext()
            for dev in usb_context.getDeviceList():
                vendor_id = dev.getVendorID()
                product_id = dev.getProductID()
                if vendor_id  == 0x1CE3 and product_id == 1:
                    self._usb_devices.append(dev)
                    bus = dev.getBusNumber()
                    port = dev.getPortNumber()
                    description = "[usb:{:03d}-{:03d}] {}".format(bus, port, dev.getProduct())
                    usb_device_descriptions.append(description)
        if len(self._usb_devices) == 0:
            self._usb_devices.append(None)
            usb_device_descriptions.append("no devices detected")

        self.addSetting(self._SETTING_DEVICE, ChoiceSetting(usb_device_descriptions, 0))
        self.addSetting(self._SETTING_COLOR_STATIC, PyQColor.fromRgb(255, 0, 255))
        self.addSetting(self._SETTING_BRIGHTNESS_STATIC, RangeSetting(0.0, 1.0, 32, 0.5))
        self.addSetting(self._SETTING_COMPRESSION_POWER, RangeSetting(0.0, 1.0, 40, 0.25))
        self.addSetting(self._SETTING_COLOR, I3TimeColorMap())
        self.addSetting(self._SETTING_INFINITE_DURATION, True)
        self.addSetting(self._SETTING_DURATION, RangeSetting(1.0, 5.0, 40, 5.0))
        self.addCleanupAction(self._cleanupDisplay)

    def _connectDevice(self):
        # Release current device before attempting new connection
        self._releaseInterface()
        if not self._usb_handle:
            dev = self._usb_devices[self.setting(self._SETTING_DEVICE)]
            if dev:
                bus = dev.getBusNumber()
                port = dev.getPortNumber()
                try:
                    self._usb_handle = dev.open()
                    self._usb_handle.claimInterface(0)
                    _log.info("Opened device [usb:{:03d}-{:03d}]".format(bus, port))
                except:
                    self._releaseInterface() # Useful call?
                    self._usb_handle = None
                    _log.warning("Could not open device [usb:{:03d}-{:03d}]".format(bus, port))

    def _cleanupDisplay(self):
        _log.info("Clearing display")
        self._connectDevice()
        # Blank display and release USB device interface
        if self._usb_handle:
            self._writeFrame(bytes(bytearray(self._FRAME_SIZE)))
        self._releaseInterface()

    def description(self):
        return "IceCube LED event display driver"

    def isValidKey(self, frame, key_idx, key):
        key_type = frame.type_name(key)
        _log.debug("key '{}' of type '{}' offered for key index {}".format(key, key_type, key_idx))
        if key_idx == 0:
            # Key may be any container of OMKeys or a mask
            # copied from Bubbles.py
            return (   ("OMKey" in key_type and not "I3ParticleID" in key_type )
                    or key_type.endswith("MapMask")
                    or key_type.endswith("MapUnion"))
        else:
            return False

    def _handleOMKeyMapTimed(self, output, omkey_pulses_map):
        color_map = self.setting(self._SETTING_COLOR)
        if self.setting(self._SETTING_INFINITE_DURATION):
            duration = None
        else:
            duration = 10**self.setting(self._SETTING_DURATION)

        # content of station_pulses: {station : [(time, charge-like)]} {int : [(float, float)]}
        station_pulses = {}

        for omkey, pulses in omkey_pulses_map:
            station = omkey.string
            om = omkey.om
            _log.debug("Data available for DOM {}-{}".format(station, om))
            if station <= 78 and om > 60 and om <= 64:
                # Ensure we're dealing with a list of pulses
                if not hasattr(pulses, "__len__"):
                    pulses = list(pulses)

                if len(pulses) > 0:
                    if station not in station_pulses:
                        station_pulses[station] = pulses
                    else:
                        # Merge two already sorted lists (merge sort)
                        station_pulses[station] = merge_lists(
                              station_pulses[station]
                            , pulses
                            , key=lambda pulse : pulse.time
                        )

        # Determine event normalisation
        max_sum_charges = 0.
        charges = {}
        for station in station_pulses:
            pulses = station_pulses[station]
            t0 = pulses[0].time
            has_npe = hasattr(pulses[0], "npe")
            has_charge = hasattr(pulses[0], "charge")

            total_charge = 0.
            if has_charge:
                charges[station] = [(pulse.time, pulse.charge) for pulse in pulses]
                total_charge += pulse.charge
            elif has_npe:
                charges[station] = [(pulse.time, pulse.npe) for pulse in pulses]
                total_charge += pulse.npe
            else:
                charges[station] = [(pulse.time, 1.0) for pulse in pulses]
                total_charge += 1.0

            if total_charge > max_sum_charges:
                max_sum_charges = total_charge

        # Iterate second time for light curves
        station_leds = {}
        normalisation = max_sum_charges
        power = self.setting(self._SETTING_COMPRESSION_POWER)
        for station in charges:
            brightness = StepFunctionFloat(0)
            pulses = charges[station]
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
                        t0s.append(t)
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

                if len(t0s) > 1:
                    color = TimeWindowColorSequence(output, t0s, color_map)
                else:
                    color = TimeWindowColor(output, t0, color_map)
            else:
                accumulated_charge = 0.0
                color = TimeWindowColor(output, t0, color_map)
                for t, q in pulses:
                    accumulated_charge += q/normalisation
                    brightness.add(accumulated_charge**power, t)

            station_leds[station] = StationLed(brightness.value, color.value)

        return station_leds

    def _handleOMKeyListStatic(self, output, omkey_list):
        color_static = self.setting(self._SETTING_COLOR_STATIC)
        brightness_static = self.setting(self._SETTING_BRIGHTNESS_STATIC)

        station_leds = {}

        for omkey in omkey_list:
            station = omkey.string
            om = omkey.om
            _log.debug("Data available for DOM {}-{}".format(station, om))
            if station <= 78 and om > 60 and om <= 64:
                if not station in station_leds:
                    station_leds[station] = StationLed(brightness_static, color_static)

        return station_leds

    def create(self, frame, output):
        self._connectDevice()

        if self._usb_handle:
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
                            self._stations = self._handleOMKeyMapTimed(output, omkey_object)
                    elif hasattr(first_value, "time"):
                        self._stations = self._handleOMKeyMapTimed(output, omkey_object)
                    else:
                        self._stations = self._handleOMKeyListStatic(output, omkey_object.keys())
            elif hasattr(omkey_object, "__len__"):
                if len(omkey_object) > 0:
                    self._stations = self._handleOMKeyListStatic(output, omkey_object)

            output.addPhantom(self._releaseInterface, self._updateDisplay)

    def _writeFrame(self, frame):
        if self._usb_handle:
            self._usb_handle.controlWrite(
                  ENDPOINT_OUT | TYPE_VENDOR | RECIPIENT_INTERFACE
                , 1
                , 0
                , 0
                , frame
            )

    def _updateDisplay(self, event_time):
        frame = bytearray(self._FRAME_SIZE)

        for station in self._stations:
            led = station - 1
            led_value = self._stations[station].getValue(event_time)
            frame[led*StationLed.DATA_LENGTH:(led+1)*StationLed.DATA_LENGTH] = led_value

        self._writeFrame(bytes(frame))

    def _releaseInterface(self):
        if self._usb_handle:
            try:
                self._usb_handle.releaseInterface(0)
                self._usb_handle.close()
            except:
                pass
            finally:
                self._usb_handle = None
                _log.info("Released USB display interface")

