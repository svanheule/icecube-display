#!/usr/bin/python3

import serial
import signal
import time

LED_COUNT = 78
BRIGHTNESS_1 = 0x80
DECAY_LENGTH = 6

TYPE_FRAME = 1


class DisplayCom:
  def __init__(self):
    self.port = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
    self.test_frame_number = 0
    self.render_frame = True
    self.direction = 1

    # Attempt to initialise port
    self.port.write(b"\x00"*10)
    time.sleep(2)

  # Render frames with decaying brightness
  def render_test(self):
    data = bytearray(4*LED_COUNT)

    # Calculate LED brightness
    brightness = BRIGHTNESS_1
    for tail in range(DECAY_LENGTH):
      led = self.test_frame_number + self.direction*tail
      if led in range(0,LED_COUNT):
        data[4*led] = 0x10
        for colour in range(1,4):
          data[4*led+colour] = brightness
      brightness >>= 2

    # Update frame number
    if self.test_frame_number == LED_COUNT and self.direction == 1:
      self.direction = -1
    elif self.test_frame_number == 0 and self.direction == -1:
      self.direction = 1

    self.test_frame_number = self.test_frame_number + self.direction

    self.send_frame(data)

  def flush_buffer(self):
    self.send_frame(b"\x00"*(LED_COUNT*4))

  def send_frame(self, data):
    if len(data) == 4*LED_COUNT:
      # Start with TYPE_FRAME
      self.port.write(b"\x41")
      # Then send frame data
      self.port.write(data)
    else:
      raise ValueError("Frame data is of incorrect length {}".format(len(data)))

  def toggle_test_mode(self):
    self.port.write(b"S")

  def interrupt_handler(self, signal, frame):
    self.render_frame = False
    self.flush_buffer()


import numpy

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


import colorsys

class Event:
  MAX_BRIGHTNESS = (2**5-1)*0.8

  def __init__(self, filename):
    stations, charges, times = numpy.loadtxt(filename, skiprows=1, unpack=True)

    self.min_charge = min(charges)
    self.max_charge = max(charges)
    # Normalise charges and gamma correct for displayability
    charges = numpy.power(charges/self.max_charge, .44)

    # Calculate time offset from first station and scale 1000ns real time to 0.5s display time
    times = 5e-3*(times - min(times)) + 0.5
    self.min_time = min(times)
    self.max_time = max(times)

    self.stations = {}
    for i,station in enumerate(stations):
      self.stations[station] = (charges[i], times[i])

    self.decay_time = 2.5
    self.stop_time = max(times) + self.decay_time

  def led_value(self, q0, t0):
    # Hue from 0 (red) to 2/3 (blue)
    hue = 2.*(t0-self.min_time)/(3*self.max_time)
    # Fully saturated
    saturation = 1.
    # Value/brightness according to normalised charge
    value = q0
    rgb = colorsys.hsv_to_rgb(hue, saturation, value)
    return numpy.array(rgb)

  def led_data(self, rgb):
    # Factor out brightness from colour
    brightness = max(1/(self.MAX_BRIGHTNESS), max(rgb))
    scaling = brightness / self.MAX_BRIGHTNESS
    rgb = [int(round(255 * c/brightness)) for c in rgb]
    return [int(round(brightness*self.MAX_BRIGHTNESS)), rgb[0], rgb[1], rgb[2]]

  def render_overview(self):
    data = bytearray(LED_COUNT*4)
    for led in range(LED_COUNT):
      station = STATION_PIXEL_MAP[led]
      if station in self.stations:
        q0, t0 = self.stations[station]
        data[4*led:4*(led+1)] = self.led_data(self.led_value(q0, t0))

    return data

  def render(self):
    # Linear rise to max brightness in 0.5s
    # Exponential decay from max brightness to 0 in 4s (tau = 0.722s)

    start_time = 0.25
    tau = self.decay_time / numpy.log(2**10)

    def brightness_curve(t, t0):
      t = t-t0
      if t <= -start_time:
        return 0
      elif t <= 0:
        return start_time * (t+start_time)
      elif t <= self.decay_time:
        return numpy.exp(-t/tau)
      else:
        return 0

    time = 0.
    while time < self.stop_time:
      data = bytearray(LED_COUNT*4)
      for led in range(LED_COUNT):
        station = STATION_PIXEL_MAP[led]
        if station in self.stations:
          q0, t0 = self.stations[station]
          rgb = brightness_curve(time, t0)*self.led_value(q0, t0)
          data[4*led:4*(led+1)] = self.led_data(rgb)

      time += 1./25
      yield data


import sys
if __name__ == "__main__":
  disp = DisplayCom()

  signal.signal(signal.SIGINT, disp.interrupt_handler)

  frame_count = 0

  if len(sys.argv) == 2:
    event = Event(sys.argv[1])

    for frame in event.render():
      disp.send_frame(frame)
      frame_count += 1
      time.sleep(1./25)

    data = event.render_overview()
    for i in range(1):
      disp.send_frame(data)
      frame_count += 1
      time.sleep(1./25)
    time.sleep(3)

    disp.flush_buffer()


  else:
    while disp.render_frame:
      disp.render_test()
      frame_count += 1
      time.sleep(1./25)

  print("displayed {} frames".format(frame_count))

