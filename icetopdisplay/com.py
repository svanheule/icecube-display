import serial
import numpy
import time
import threading

from .geometry import LED_COUNT


class DisplayCom:
  MAX_BRIGHTNESS = 2**5-1

  def __init__(self, device="/dev/ttyACM0"):
    self.port = serial.Serial(device, 115200, timeout=0.1)
    self._connect()
    self._lock = threading.Lock()

  def acquire(self):
    self._lock.acquire()

  def release(self):
    self._lock.release()

  def _connect(self, waiting_time=2):
    # Ask for ID (command "I") and fail on timeout or incompatibility
    self.port.write(b"I")
    waiting = 0
    while self.port.inWaiting() < 8 and waiting < waiting_time:
      time.sleep(0.1)
      waiting += 0.1
      self.port.write(b"I")
    if self.port.inWaiting() >= 8:
      result = self.port.read(8)
      if result != b"IT78:1:0":
        raise Exception("Incompatible device type {}".format(result))
    else:
      raise Exception("connection to display timed out")

  def _led_data(self, rgb):
    # Factor out brightness from colour
    rgb[0] *= 0.85
    rgb[2] *= 0.95
    brightness = max(1/self.MAX_BRIGHTNESS, max(rgb))
    scaling = brightness / self.MAX_BRIGHTNESS
    rgb = [int(round(255 * c/brightness)) for c in numpy.power(rgb, 2.2)]
    return [int(round(brightness*self.MAX_BRIGHTNESS)), rgb[0], rgb[1], rgb[2]]

  def _send_frame(self, frame):
      # Start with TYPE_FRAME
      self.port.write(b"\x41")
      self.port.write(frame)

  def flush_buffer(self):
    self._send_frame(b"\x00"*(LED_COUNT*4))

  # Takes a (LEDCOUNT, 3)-sized 2D array of RGB floating point values in [0,1]
  def send_frame(self, data):
    if len(data) == LED_COUNT:
      # Normalise RGB values
      frame = bytearray(LED_COUNT*4)
      for i,rgb in enumerate(data):
        frame[4*i:4*(i+1)] = self._led_data(numpy.copy(rgb))
      # Then send frame data
      self._send_frame(frame)
    else:
      raise ValueError("Frame data is of incorrect length {}".format(len(data)))

  def toggle_snake_test_mode(self):
    self.port.write(b"S")

  def toggle_ring_test_mode(self):
    self.port.write(b"R")

