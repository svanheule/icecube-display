#!/usr/bin/python3

import numpy
import sys
import json
import colorsys

from icetopdisplay import DisplayCom
from icetopdisplay.geometry import LED_COUNT, coordinates_to_pixel
from render import Renderer, RenderInterrupt

# Import GStreamer
import gi
gi.require_version('Gst', '1.0')
gi.require_version('GLib', '2.0')

from gi.repository import GObject, Gst, GLib


class AudioSpectrum(Renderer):
  # Setup display specifics
  AUDIO_RATE = 44100
  BANDS = 10
  FPS = 25
  MIN_AMPLITUDE = -60
  MAX_AMPLITUDE = -15

  HUE_START = 180/360 # blue
  HUE_END = 0 # red
  HUE_DELTA = HUE_END - HUE_START

  MAX_BRIGHTNESS = 2**2

  def __init__(self, displaycom, bandwidth=44100, min_amplitude=-60, max_amplitude=0):
    super().__init__(displaycom)
    self.bands = max(int(44100/bandwidth * self.BANDS), self.BANDS)
    self.min_amplitude = min_amplitude
    self.max_amplitude = max_amplitude

    self.left = min_amplitude*numpy.ones(self.BANDS)
    self.right = min_amplitude*numpy.ones(self.BANDS)
    self.last_frame = numpy.zeros((LED_COUNT, 3))

    # set up audio pipeline
    # Capture sound card output
#    src = Gst.ElementFactory.make("alsasrc", None)
#    src.set_property("device", "alsa_output.pci-0000_00_1b.0.analog-stereo.monitor")
    src = Gst.ElementFactory.make("filesrc", "input")
    src.set_property("location", sys.argv[1])
    decode = Gst.ElementFactory.make("mad", "decode")
    convert = Gst.ElementFactory.make("audioconvert", "convert")

    # Raw audio filter
    caps_string = "audio/x-raw, format=(string)S32LE, rate=(int){}, channels=2, layout=(string)interleaved".format(self.AUDIO_RATE)
    caps = Gst.Caps.from_string(caps_string)
    audio_filter = Gst.ElementFactory.make("capsfilter", "filter")

    # Create spectrum element to get 25 (6×5) spectra per second
    spectrum = Gst.ElementFactory.make("spectrum", "spectrum-analyser")
    spectrum.set_property("bands", self.BANDS) # Set number of frequency bands
    spectrum.set_property("interval", int(1e9/self.FPS)) # Get the spectrum every 4e7 ns
    spectrum.set_property("threshold", self.MIN_AMPLITUDE) # Set the amplitude's lower bound
    spectrum.set_property("message-magnitude", True) # Return the signal magnitude (in dB)
    spectrum.set_property("message-phase", False) # Don't return the signal phase
    spectrum.set_property("multi-channel", True) # Keep left and right channel separate
    spectrum.set_property("post-messages", True) # Make sure messages are actually posted

    sink = Gst.ElementFactory.make("fakesink", "output")

    self.pipeline = Gst.Pipeline.new("icetop-display-vu")
    self.pipeline.add(src)
    self.pipeline.add(decode)
    self.pipeline.add(convert)
    self.pipeline.add(audio_filter)
    self.pipeline.add(spectrum)
    self.pipeline.add(sink)

    src.link(decode)
    decode.link(convert)
    convert.link(audio_filter)
    audio_filter.link(spectrum)
    spectrum.link(sink)

    bus = self.pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect("message", self.message_handler)

  def start(self):
    self.pipeline.set_state(Gst.State.PLAYING)
    super().start()

  def cancel(self):
    self.pipeline.set_state(Gst.State.NULL)
    super().cancel()

  def fill_band(self, data, band, amplitude, origin, col_led_count, direction):
    amplitude = min(amplitude, self.max_amplitude)
    delta_amplitude = self.max_amplitude-self.min_amplitude
    db_interval = delta_amplitude/col_led_count

    db_shifted = amplitude-self.min_amplitude
    distance = db_shifted/db_interval

    top = min(col_led_count-1, numpy.floor(distance))
    offset = [direction[0]*top+band, direction[1]*top]
    position = numpy.add(origin, offset)
    pixel = coordinates_to_pixel(position)

    hue = db_shifted/delta_amplitude*self.HUE_DELTA + self.HUE_START
    saturation = 1.

    db_interval_fill = (db_shifted - db_interval*top)/db_interval
    value = db_interval_fill
    data[pixel] = colorsys.hsv_to_rgb(hue, saturation, value)

    # Fill below top pixel
    for row in range(int(top)):
      position = numpy.add(origin, [direction[0]*row+band, direction[1]*row])
      pixel = coordinates_to_pixel(position)
      hue = (row+1)/col_led_count*self.HUE_DELTA + self.HUE_START
      data[pixel] = colorsys.hsv_to_rgb(hue, saturation, 1.)


  def next_frame(self):
    left_origin = [1,5]
    left_direction = [1,1]
    left_lengths = [5, 5, 5, 5, 4, 4, 4, 3, 2, 1]

    right_origin = [0,4]
    right_direction = [0,-1]
    right_lengths = [5, 5, 5, 5, 5, 5, 4, 3, 2, 1]

    # Copy to prevent mixing of data
    left = self.left
    right = self.right
    data = numpy.zeros((LED_COUNT, 3))
    # Render all the things!
    # Amplitude will range from MIN_AMPLITUDE to 0 dB, so divide this in led-count ranges per line
    # Use the hue range from 120 (green) to 0 deg (red).
    for band, length in enumerate(left_lengths):
      self.fill_band(data, band, left[band], left_origin, length, left_direction)

    for band, length in enumerate(right_lengths):
      self.fill_band(data, band, right[band], right_origin, length, right_origin)

    return data

  def message_handler(self, bus, message):
    print("got message")
    structure = message.get_structure()
    name = structure.get_name()

    if message.type == Gst.MessageType.ERROR:
      err, debug = message.parse_error()
      print("Error: {}, {}".format(err, debug))

    if name == "spectrum":
      # XXX Workarond: http://lists.freedesktop.org/archives/gstreamer-devel/2015-August/054119.html
      s = message.get_structure().to_string()
      sep = 'magnitude=(float)'
      magnitudes = json.loads(s[s.find(sep)+len(sep):].translate(bytes.maketrans(b'<>;',b'[] ')))
      self.left = magnitudes[0]
      self.right = magnitudes[1]


if __name__ == "__main__":
  GObject.threads_init()
  Gst.init(None)

  disp = DisplayCom()
  renderer = AudioSpectrum(disp)
  int_handler = RenderInterrupt(renderer)
  renderer.start()

  # TODO GLib Mainloop

  disp.acquire()
  disp.flush_buffer()
  disp.release()

