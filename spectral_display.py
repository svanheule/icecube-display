#!/usr/bin/python3

import numpy
import sys
import colorsys

from render import DisplayCom

# Import GStreamer
import gi
gi.require_version('Gst', '1.0')

from gi.repository import GObject, Gst

GObject.threads_init()
Gst.init(None)


# Setup display specifics
AUDIO_RATE = 44100
BANDS = 10
FPS = 25
MIN_AMPLITUDE = -60

PIXEL_COORDINATES = [
    [5,0], [4,0], [3,0], [2,0], [1,0], [0,0]
  , [0,1], [1,1], [2,1], [3,1], [4,1], [5,1], [6,1]
  , [7,2], [6,2], [5,2], [4,2], [3,2], [2,2], [1,2], [0,2]
  , [0,3], [1,3], [2,3], [3,3], [4,3], [5,3], [6,3], [7,3], [8,3]
  , [9,4], [8,4], [7,4], [6,4], [5,4], [4,4], [3,4], [2,4], [1,4], [0,4]
  , [1,5], [2,5], [3,5], [4,5], [5,5], [6,5], [7,5], [8,5], [9,5], [10,5]
  , [10,6], [9,6], [8,6], [7,6], [6,6], [5,6], [4,6], [3,6], [2,6]
  , [3,7], [4,7], [5,7], [6,7], [7,7], [8,7], [9,7], [10,7]
  , [10,8], [9,8], [8,8], [7,8], [6,8], [5,8], [4,8]
  , [5,9], [6,9], [7,9], [8,9]
]
LED_COUNT = len(PIXEL_COORDINATES)
MAX_BRIGHTNESS = 2**2

# Determine mapping of pixels to station
ROW_START = [1, 7, 14, 22, 31, 41, 51, 60, 68, 75, 79]
STATION_PIXEL_MAP = numpy.empty(78, dtype=int)
for row in range(len(ROW_START)-1):
  direction = int((-1)**(row+1))
  if direction == 1:
    start = ROW_START[row]
  else:
    start = ROW_START[row+1]-1

  pixel_start = ROW_START[row]
  pixel_end = ROW_START[row+1]
  station = start

  for pixel in range(pixel_start-1, pixel_end-1):
    STATION_PIXEL_MAP[pixel] = station
    station += direction

def get_station_key(location):
  return (int(location[0]) << 4) | int(location[1])

PIXEL_STATION_MAP = {}
for i,location in enumerate(PIXEL_COORDINATES):
  PIXEL_STATION_MAP[get_station_key(location)] = i


def lookup_pixel(location):
  return PIXEL_STATION_MAP[get_station_key(location)]


def led_data(rgb):
  # Factor out brightness from colour
  gamma = 2.2
  rgb = numpy.power(rgb, gamma)
  brightness = max(1/(MAX_BRIGHTNESS), max(rgb))
  scaling = brightness / MAX_BRIGHTNESS
  rgb = [int(round(255 * c/brightness)) for c in rgb]
  return [int(round(brightness*MAX_BRIGHTNESS)), rgb[0], rgb[1], rgb[2]]


HUE_START = 180/360 # blue
HUE_END = 0 # red
HUE_DELTA = HUE_END - HUE_START

def set_colour_pos(data, band, amplitude, origin, lengths, direction):
  col_led_count = lengths[band]
  db_shifted = amplitude-MIN_AMPLITUDE
  distance = abs(db_shifted/MIN_AMPLITUDE)*col_led_count

  top = min(col_led_count-1, numpy.floor(distance))
  offset = [direction[0]*top+band, direction[1]*top]
  position = numpy.add(origin, offset)
  pixel = lookup_pixel(position)

  hue = db_shifted/(-MIN_AMPLITUDE)*HUE_DELTA + HUE_START
  saturation = 1.

  db_interval = abs(MIN_AMPLITUDE/col_led_count)
  db_interval_fill = (db_shifted - db_interval*top)/db_interval
  value = db_interval_fill
  colour = colorsys.hsv_to_rgb(hue, saturation, value)
  data[4*pixel:4*(pixel+1)] = led_data(colour)

  # Fill below top pixel
  for row in range(int(top)):
    position = numpy.add(origin, [direction[0]*row+band, direction[1]*row])
    pixel = lookup_pixel(position)
    hue = (row+1)/col_led_count*HUE_DELTA + HUE_START
    value = 1.
    data[4*pixel:4*(pixel+1)] = led_data(colorsys.hsv_to_rgb(hue, saturation, value))


def render_spectrum(left, right):
  LEFT_ORIGIN = [1,5]
  LEFT_DIRECTION = [1,1]
  LEFT_LENGTHS = [5, 5, 5, 5, 4, 4, 4, 3, 2, 1]

  RIGHT_ORIGIN = [0,4]
  RIGHT_DIRECTION = [0,-1]
  RIGHT_LENGTHS = [5, 5, 5, 5, 5, 5, 4, 3, 2, 1]

  data = bytearray(LED_COUNT*4)
  # Render all the things!
  # Amplitude will range from MIN_AMPLITUDE to 0 dB, so divide this in led-count ranges per line
  # Use the hue range from 120 (green) to 0 deg (red).
  for band, amplitude in enumerate(left):
    set_colour_pos(data, band, amplitude, LEFT_ORIGIN, LEFT_LENGTHS, LEFT_DIRECTION)

  for band, amplitude in enumerate(right):
    set_colour_pos(data, band, amplitude, RIGHT_ORIGIN, RIGHT_LENGTHS, RIGHT_DIRECTION)

  return data


def print_icetop(data):
  def transform(position):
    x = 2*position[0] - position[1] + 4
    y = position[1]
    return [x,y]

  line = 0
  column = 0
  station = 0
  while station < LED_COUNT and line < 10:
    pixel = STATION_PIXEL_MAP[station]-1
    x, y = transform(PIXEL_COORDINATES[pixel])
    if x == column:
      print(int(bool(data[pixel*4])), end="")
      station += 1
    else:
      print(" ", end="")

    if y != line or (station == LED_COUNT) or column > 24:
      print("")
      line += 1
      column = 0
    else:
      column += 1

  print("\033[{}A".format(line), end='\r')
#  print("")


# Spectrun handler
import json
pipeline = None
disp = DisplayCom()
last_frame = bytearray(4*LED_COUNT)

def message_handler(bus, message):
  global pipeline, disp, last_frame
  structure = message.get_structure()
  name = structure.get_name()

  if message.type == Gst.MessageType.ERROR:
    err, debug = message.parse_error()
    print("Error: {}, {}".format(err, debug))

#  try:
  if name == "spectrum":
    # XXX Workarond: http://lists.freedesktop.org/archives/gstreamer-devel/2015-August/054119.html
    s = message.get_structure().to_string()
    sep = 'magnitude=(float)'
    magnitudes = json.loads(s[s.find(sep)+len(sep):].translate(bytes.maketrans(b'<>;',b'[] ')))
    data = render_spectrum(magnitudes[0], magnitudes[1])
#    print_icetop(data)
    last_frame = bytearray(int((p/8 + d)/1.125) for p,d in zip(last_frame,data))
    disp.send_frame(last_frame)
#  except KeyError as e:
#    key = int(str(e))
#    print("Key error: {},{}".format(key>>4, key&0xf))
#    pipeline.set_state(Gst.State.NULL)
#    sys.exit(-1)
#  except Exception as e:
#    print(repr(e))
#    pipeline.set_state(Gst.State.NULL)
#    sys.exit(-1)


if __name__ == "__main__":
  # Capture sound card output
#  src = Gst.ElementFactory.make("pulsesrc", None)
#  src.set_property("device", "alsa_output.pci-0000_00_1b.0.analog-stereo.monitor")
  src = Gst.ElementFactory.make("filesrc", "input")
  if len(sys.argv) > 1:
    location = sys.argv[1]
  else:
    location = "/home/sander/Muziek/Chromeo/Business Casual/02  - I'm Not Contagious.mp3"
#    location = "/home/sander/Muziek/Metronomy/The English Riviera/04 The Look.mp3"
  print("Playing {}".format(location))

  src.set_property("location", location)
  decode = Gst.ElementFactory.make("mad", "decode")
  convert = Gst.ElementFactory.make("audioconvert", "convert")

  # Raw audio filter
  caps_string = "audio/x-raw, format=(string)S32LE, rate=(int){}, channels=2, layout=(string)interleaved".format(AUDIO_RATE)
  caps = Gst.Caps.from_string(caps_string)
  audio_filter = Gst.ElementFactory.make("capsfilter", "filter")

  # Create spectrum element to get 25 (6×5) spectra per second
  spectrum = Gst.ElementFactory.make("spectrum", "spectrum-analyser")
  spectrum.set_property("bands", BANDS) # Set number of frequency bands
  spectrum.set_property("interval", int(1e9/FPS)) # Get the spectrum every 4e7 ns
  spectrum.set_property("threshold", MIN_AMPLITUDE) # Set the amplitude's lower bound
  spectrum.set_property("message-magnitude", True) # Return the signal magnitude (in dB)
  spectrum.set_property("message-phase", False) # Don't return the signal phase
  spectrum.set_property("multi-channel", True) # Keep left and right channel separate

  sink = Gst.ElementFactory.make("alsasink", "output")

  pipeline = Gst.Pipeline.new("icetop-display-vu")
  pipeline.add(src)
  pipeline.add(decode)
  pipeline.add(convert)
  pipeline.add(audio_filter)
  pipeline.add(spectrum)
  pipeline.add(sink)

  src.link(decode)
  decode.link(convert)
  convert.link(audio_filter)
  audio_filter.link(spectrum)
  spectrum.link(sink)

  bus = pipeline.get_bus()
  bus.add_signal_watch()
  bus.connect("message", message_handler)

  # Set state to PLAYING to start rendering
  pipeline.set_state(Gst.State.PLAYING)

  # Wait for error or end-of-stream
  msg = bus.timed_pop_filtered(Gst.CLOCK_TIME_NONE, Gst.MessageType.ANY)
  while msg.type != Gst.MessageType.ERROR and msg.type != Gst.MessageType.EOS:
    message_handler(bus, msg)
    msg = bus.timed_pop_filtered(Gst.CLOCK_TIME_NONE, Gst.MessageType.ANY)

  pipeline.set_state(Gst.State.NULL)

