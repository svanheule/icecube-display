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
  sys.path.append("/usr/local/lib/python2.7/dist-packages/libusb1-1.4.1-py2.7.egg")
  from usb1 import USBContext, ENDPOINT_OUT, TYPE_VENDOR, RECIPIENT_INTERFACE
  # Add icetopdisplay module to path
  # FIXME Ensure the icetopdisplay module can be loaded instead of the submodules
  sys.path.append("/home/sjvheule/projects/be.ugent/pocket-icetop/icetopdisplay")
  from led_format import FormatAPA102
  from geometry import LED_COUNT
except:
  USBContext = None
  _log.error("Failed to import python-libusb1")

from icecube.shovelart import PyArtist
from icecube.shovelart import ChoiceSetting, I3TimeColorMap
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
    while i < len(left):
        merged.append(left[i])
        i = i+1
    while j < len(right):
        merged.append(right[j])
        j = j+1

    return merged

class StationLed(object):
    def __init__(self, brightness, color):
        if not hasattr(brightness, "__call__"):
            self._brightness = (lambda time : brightness)
        else:
            self._brightness = brightness

        if not hasattr(color, "__call__"):
            self._color = (lambda time : color)
        else:
            self._color = color

    def getValue(self, time):
        brightness = min(0.8, self._brightness(time)) # Clip brightness
        color = self._color(time)
        alpha = float(color.alpha)/255
        value = [comp*brightness*alpha for comp in color.rgbF()]
        return FormatAPA102.float_to_led_data(value)


class LedDisplay(PyArtist):
    numRequiredKeys = 1
    _SETTING_COLOR = "colormap"
    _SETTING_DURATION = "duration"

    def __init__(self):
        PyArtist.__init__(self)

        # USB Device handle field
        self._usb_handle = None
        self._stations = {}

        # TODO Settings
        # * ChoiceSetting: select USB device; hotplugging not possible!
        # * I3TimeColorMap: time to RGB mapping
        # * RangeSetting: charge compression
        # * RangeSetting: charge normalisation
#        self.defineSettings( { "brightness": RangeSetting(0.0, 1.0, 100, 0.2),
#                               "scale": RangeSetting(-2.0, 1.0, 100, -0.65),
#                               "power": RangeSetting(.01,.5, 100, 0.157),
#                               "colormap": I3TimeColorMap() } )

#        self.addSetting("device", ChoiceSetting(self.device_choices_descriptions, 0))
        self.addSetting(self._SETTING_COLOR, I3TimeColorMap())
#        self.addSetting(self._SETTING_DURATION, RangeSetting( 1.0, 5.0, 40, 5.0 ))

    def description(self):
        return "IceCube LED event display driver"

    def isValidKey(self, frame, key_idx, key):
        key_type = frame.type_name(key)
        if key_idx == 0:
            _log.debug("key '{}' of type '{}' offered for key index {}".format(key, key_type, key_idx))
            # Key may be any container of OMKeys or a mask
            # copied from Bubbles.py
            return (   ("OMKey" in key_type and not "I3ParticleID" in key_type )
                    or key_type.endswith("MapMask")
                    or key_type.endswith("MapUnion"))
        else:
            return False

    def create(self, frame, output):
        # Get list of devices as late as possible, because we can't hook into changes of
        # ChoiceSetting's value
        if USBContext:
            usb_context = USBContext()
            for dev in usb_context.getDeviceList():
                # Be strict on device selection since getting feedback from the GUI is a bit
                # sub-optimal
                vendor_id = dev.getVendorID()
                product_id = dev.getProductID()
                try:
                    serial = dev.getSerialNumber()
                except:
                    serial = None
                if vendor_id  == 0x1CE3 and product_id == 1 and serial == "ICD-IT-001-0001":
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

        if self._usb_handle:
            color_map = self.setting(self._SETTING_COLOR)
            
            (frame_key,) = self.keys()
            pulse_series = frame[frame_key]

            # If we are displaying a mask/union, ensure pulse_series is a I3RecoPulseSeries object
            if isinstance(pulse_series, (I3RecoPulseSeriesMapMask, I3RecoPulseSeriesMapUnion)):
                pulse_series = pulse_series.apply(frame)

#            def has_time(value):
#                return hasattr(value, "time")

            station_pulses = {}

            if hasattr(pulse_series, "keys"): # I3Map of OMKeys
                for omkey, pulses in pulse_series:
                    _log.debug("Data available for DOM {}-{}".format(omkey.string, omkey.om))
                    if hasattr(pulses, "__len__") and len(pulses) > 0:
                        station = omkey.string
                        om = omkey.om
                        if station <= 78 and om > 60 and om <= 64:
                            if station not in station_pulses:
                                station_pulses[station] = list(pulses)
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

                if has_charge:
                    charges[station] = [(pulse.time, pulse.charge) for pulse in pulses]
                elif has_npe:
                    charges[station] = [(pulse.time, pulse.npe) for pulse in pulses]
                else:
                    charges[station] = [(pulse.time, 1.0) for pulse in pulses]

                sum_charges = sum(zip(*charges[station])[1])
                if sum_charges > max_sum_charges:
                    max_sum_charges = sum_charges

            # Iterate second time for light curves
            self._stations = {}
            normalisation = 1./max_sum_charges
            power = 0.15
            for station in charges:
                t0, q0 = charges[station][0]
                brightness = StepFunctionFloat(0)
                accumulated_charge = 0
                for time, charge in charges[station]:
                    accumulated_charge += (charge*normalisation)**power
                    brightness.add(accumulated_charge, time)
                color = TimeWindowColor(output, t0, color_map)
                self._stations[station] = StationLed(brightness.value, color.value)

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
        frame = bytearray(LED_COUNT*FormatAPA102.LENGTH)

        for station in self._stations:
            led = station - 1
            led_value = self._stations[station].getValue(event_time)
            frame[led*FormatAPA102.LENGTH:(led+1)*FormatAPA102.LENGTH] = led_value

        self._writeFrame(bytes(frame))

    def _releaseInterface(self):
        if self._usb_handle:
            # Blank display and release USB device interface
            self._writeFrame(bytes(bytearray(LED_COUNT*FormatAPA102.LENGTH)))
            self._usb_handle.releaseInterface(0)
            self._usb_handle.close()
            self._usb_handle = None
            _log.info("Released USB display interface")

