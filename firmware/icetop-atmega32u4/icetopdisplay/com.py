import serial
import numpy
import time
import threading

from .geometry import LED_COUNT

class DisplayCom:
  def __init__(self):
    self._connect()
    self._lock = threading.Lock()

  def acquire(self):
    self._lock.acquire()

  def release(self):
    self._lock.release()

  def _connect(self, waiting_time):
    raise NotImplementedError

  def _send_frame(self, frame):
    raise NotImplementedError

  # Send a frame buffer to the display.
  # Use LedFormat to convert 3-tuples of floats to the correct buffer format.
  def send_frame(self, data):
    data = numpy.array(data, dtype=numpy.uint8).reshape(LED_COUNT*4)
    frame = bytearray(data)
    self._send_frame(frame)

  def flush_buffer(self):
    self._send_frame(b"\x00"*(LED_COUNT*4))


class DisplayComUart(DisplayCom):
  def __init__(self, device="/dev/ttyACM0"):
    self.port = serial.Serial(device, 115200, timeout=0.1)
    super().__init__()

  def _connect(self, waiting_time=2):
    # Ask for ID (command "I") and fail on timeout or incompatibility
    self.port.write(b"LI")
    waiting = 0
    poll_time = 0.5
    while self.port.inWaiting() < 8 and waiting < waiting_time:
      time.sleep(poll_time)
      waiting += poll_time
      self.port.write(b"LI")
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

  def toggle_snake_test_mode(self):
    self.port.write(b"S")

  def toggle_ring_test_mode(self):
    self.port.write(b"R")


# USB communication support
import usb1

class DisplayComUsb(DisplayCom):
  def __init__(self):
    self.usb_context = usb1.USBContext()
    self._handle = None
    self._device = None
    super().__init__()

  def acquire(self):
    super().acquire()
    self._handle = self._device.open()

  def release(self):
    self._handle.close()
    self._handle = None
    super().release()

  def _connect(self, waiting_time=0):
    devices = self.usb_context.getDeviceList(skip_on_error=True)
    self._device = None
    # Find IceCube device
    for dev in devices:
      if dev.getVendorID() == 0x1CE3:
        self._device = dev

  def _send_frame(self, frame):
    if self._handle:
      self._handle.controlWrite(
          usb1.ENDPOINT_OUT | usb1.TYPE_VENDOR | usb1.RECIPIENT_INTERFACE
        , 1
        , 0
        , 0
        , bytes(frame)
      )

