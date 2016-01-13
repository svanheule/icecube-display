#!/usr/bin/python3

import time, threading
from icetopdisplay import DisplayCom, LedFormat
from icetopdisplay.geometry import pixel_to_station, LED_COUNT

class Renderer:
  def __init__(self, displaycom, frame_rate=25):
    self._timer = None
    self.halt = True
    self._com = displaycom
    self.interval = 1/frame_rate

  def start(self):
    self._halt = False
    self.frame_number = 0
    self.display_time = 0
    self._com.acquire()
    self._start_time = time.time()
    self._setup_next_frame()

  def cancel(self):
    self._halt = True
    if self._timer is not None:
      self._timer.cancel()
      self._timer = None
    self._com.release()

  def _setup_next_frame(self):
    next_time = self._start_time + self.display_time + self.interval
    self._timer = threading.Timer(next_time - time.time(), self._send_next_frame)
    self._timer.start()
    self.display_time = self.frame_number*self.interval
    self.frame_number += 1

  def _send_next_frame(self):
    try:
      if not self._halt:
        self._setup_next_frame()
      data = self.next_frame()
      self._com.send_frame(data)
      # Set up next frame transmission
    except StopIteration:
      self.cancel()

  def next_frame(self):
    pass

class TestRenderer(Renderer):
  DECAY_LENGTH = 6

  def __init__(self, displaycom, frame_rate=25):
    super().__init__(displaycom, frame_rate)
    self.position = 0
    self.direction = 1

  # Render frames with decaying brightness
  def next_frame(self):
    data = numpy.zeros((LED_COUNT, 4))

    # Calculate LED brightness
    brightness = 0.5
    for tail in range(self.DECAY_LENGTH):
      led = self.position - self.direction*tail
      if led in range(LED_COUNT):
        data[led] = LedFormat.float_to_led_data([brightness]*3)
      brightness /= 2

    # Update frame number
    if self.position == LED_COUNT-1 and self.direction == 1:
      self.direction = -1
    elif self.position == 0 and self.direction == -1:
      self.direction = 1

    self.position = self.position + self.direction

    return data

import numpy
import colorsys

class EventRenderer(Renderer):
  TIME_STRETCH = 4e-3 # in s/ns
  TIME_RISE = 0.5 # in s
  TIME_DECAY = 3.5

  def __init__(self, displaycom, filename, frame_rate=25, overview_time=3):
    super().__init__(displaycom, frame_rate)
    stations, charges, times = numpy.loadtxt(filename, skiprows=1, unpack=True)

    charges = numpy.log10(charges+1)

    self.min_charge = min(charges)
    self.max_charge = max(charges)
    # Normalise charges and compress for displayability
    charges = numpy.power(charges/self.max_charge, 2)

    # Calculate time offset from first station and scale real time to display time
    times = self.TIME_STRETCH*(times - min(times)) + self.TIME_RISE
    self.min_time = min(times)
    self.max_time = max(times)

    self.stations = {}
    for i,station in enumerate(stations):
      # Discard infill stations
      if station <= 78:
        self.stations[station] = (charges[i], times[i])

    self.stop_time = max(times) + self.TIME_DECAY
    self.overview_time = overview_time

    self.tau = self.TIME_DECAY / numpy.log(2**9)
    self.colours = self.render_overview()

  def led_value(self, q0, t0):
    # Hue from 0 (red) to 2/3 (blue)
    hue = 2.*(t0-self.min_time)/(3*(self.max_time-self.min_time))
    # Fully saturated
    saturation = 1.
    # Value/brightness according to normalised charge
    value = q0
    return numpy.array(colorsys.hsv_to_rgb(hue, saturation, value))

  def render_overview(self):
    data = numpy.zeros((LED_COUNT, 3))
    for led in range(LED_COUNT):
      station = pixel_to_station(led)
      if station in self.stations:
        q0, t0 = self.stations[station]
        data[led] = self.led_value(q0, t0)

    return data

  def brightness_curve(self, t, t0):
    # Linear rise to max brightness in TIME_RISE
    # Exponential decay from max brightness to 0 in TIME_DECAY
    t = t-t0
    if t <= -self.TIME_RISE:
      return 0
    elif t <= 0:
      return 1 + t/self.TIME_RISE
    elif t <= self.TIME_DECAY:
      return numpy.exp(-t/self.tau)
    else:
      return 0

  def next_frame(self):
    if self.display_time < self.stop_time:
      data = numpy.zeros((LED_COUNT, 4))
      for led in range(LED_COUNT):
        station = pixel_to_station(led)
        if station in self.stations:
          q0, t0 = self.stations[station]
          data[led] = LedFormat.float_to_led_data(self.brightness_curve(self.display_time, t0)*self.colours[led])
      return data
    elif self.display_time < self.stop_time+self.overview_time:
      return self.colours
    else:
      raise StopIteration


import sys
import signal

class RenderInterrupt:
  def __init__(self, renderer):
    self._renderer = renderer
    signal.signal(signal.SIGINT, self._handler)

  def _handler(self, signal, frame):
    self._renderer.cancel()


if __name__ == "__main__":
  disp = DisplayCom()

  if len(sys.argv) == 2:
    renderer = EventRenderer(disp, sys.argv[1])
  else:
    renderer = TestRenderer(disp)

  RenderInterrupt(renderer)
  renderer.start()

  disp.acquire()
  disp.flush_buffer()
  disp.release()

