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
    self.frame_number = 0
    self.render_frame = True
    self.direction = 1

    signal.signal(signal.SIGINT, self.interrupt_handler)

  # Render frames with decaying brightness
  def render_test(self):
    data = bytearray(3*LED_COUNT)

    # Calculate LED brightness
    brightness = BRIGHTNESS_1
    for tail in range(DECAY_LENGTH):
      led = self.frame_number + self.direction*tail
      if led in range(0,LED_COUNT):
        for colour in range(0,3):
          data[3*led+colour] = brightness
      brightness >>= 2

    # Update frame number
    if self.frame_number == LED_COUNT and self.direction == 1:
      self.direction = -1
    elif self.frame_number == 0 and self.direction == -1:
      self.direction = 1

    self.frame_number = self.frame_number + self.direction

    self.send_frame(data)

  def flush_buffer(self):
    self.send_frame(b"\x00"*(LED_COUNT*3))

  def send_frame(self, data):
    # Start with TYPE_FRAME
    self.port.write(b"\x41")
    # Then send frame data
#    print(len(data))
#    transmitted = 0
#    while transmitted < len(data):
#      self.port.write(data[transmitted:transmitted+60])
#      transmitted += 60
#      time.sleep(1./100)
    self.port.write(data)

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


class Event:
  # TODO charge range, time range
  # TODO gamma correction

  def __init__(self, filename):
    stations, charges, times = numpy.loadtxt(filename, skiprows=1, unpack=True)

    min_charge = min(charges)
    max_charge = max(charges)

    # Calculate time offset from first station and scale 1000ns real time to 0.5s display time
    times = 5e-3*(times - min(times)) + 0.5
    # Normalise charges and gamma correct for displayability
    charges = numpy.floor(255*numpy.power(charges/max_charge, .4))

    self.stations = {}
    for i,station in enumerate(stations):
      self.stations[station] = (charges[i], times[i])

    self.decay_time = 3.5
    self.stop_time = max(times) + self.decay_time

  def __iter__(self):
    return EventIterator(self.stations, self.stop_time)

  def render(self):
    # Linear rise to max brightness in 0.5s
    # Exponential decay from max brightness to 0 in 4s (tau = 0.722s)

    start_time = 0.25
    tau = self.decay_time / numpy.log(255)

    def light_curve(t, q0, t0):
      t = t-t0
      if t <= -start_time:
        return 0
      elif t <= 0:
        return q0/start_time * (t + start_time)
      elif t <= 4:
        return q0*numpy.exp(-t/tau)
      else:
        return 0

    time = 0.
    while time < self.stop_time:
      data = bytearray(LED_COUNT*3)
      for led in range(LED_COUNT):
        station = STATION_PIXEL_MAP[led]
        if station in self.stations:
          q0, t0 = self.stations[station]
          brightness = int(light_curve(time, q0, t0))
          # TODO Use colour for timing
          for colour in range(3):
            data[3*led + colour] = brightness

      time += 1./25
      yield data


import sys
if __name__ == "__main__":
  disp = DisplayCom()

  frame_count = 0

  if len(sys.argv) == 2:
    event = Event(sys.argv[1])

    for frame in event.render():
      disp.send_frame(frame)
      frame_count += 1
      time.sleep(1./25)

    disp.flush_buffer()


  else:
    while disp.render_frame:
      disp.render_test()
      frame_count += 1
      time.sleep(1./25)

  print("displayed {} frames".format(frame_count))

