import serial
import numpy
import time
import threading

from .geometry import LED_COUNT


class DisplayCom:

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

  def _send_frame(self, frame):
      # Start with TYPE_FRAME
      self.port.write(b"\x41")
      self.port.write(frame)

  def flush_buffer(self):
    self._send_frame(b"\x00"*(LED_COUNT*4))

  # Send a frame buffer to the display.
  # Use LedFormat to convert 3-tuples of floats to the correct buffer format.
  def send_frame(self, data):
    data = numpy.array(data, dtype=numpy.uint8).reshape(LED_COUNT*4)
    frame = bytearray(data)
    self._send_frame(frame)

  def toggle_snake_test_mode(self):
    self.port.write(b"S")

  def toggle_ring_test_mode(self):
    self.port.write(b"R")

