import usb1, struct

cxt = usb1.USBContext()
devs = cxt.getDeviceList(skip_on_error=True)

ic_dev = None
for dev in devs:
  if dev.getVendorID() == 0x1CE3:
    ic_dev = dev

if not ic_dev is None:

  handle = ic_dev.open()

  VENDOR_IN = usb1.ENDPOINT_IN | usb1.TYPE_VENDOR | usb1.RECIPIENT_INTERFACE
  REQUEST_DISPLAY_PROPERTIES = 2

  # Read only properties header: properties length
  data = handle.controlRead(VENDOR_IN, REQUEST_DISPLAY_PROPERTIES, 0, 0, 2)
  # Read full TLV list, skip header this time
  size = struct.unpack('<H', data)[0]
  data = handle.controlRead(VENDOR_IN, REQUEST_DISPLAY_PROPERTIES, 0, 0, size)[2:]

  DP_TYPE_INFORMATION_RANGE = 2

  i = 0
  led_count = 0
  while i < len(data):
    info_type, info_length = struct.unpack('<2B', data[i:i+2])
    if info_type == DP_TYPE_INFORMATION_RANGE:
      start, end = struct.unpack('<2B', data[i+2:i+4])
      led_count = end - start + 1
    i += 2 + info_length

  frame = [0]*led_count*4
  for station in range(led_count):
    frame[4*station:4*(station+1)] = [0x10, 255, 0, 0]

  VENDOR_OUT = usb1.ENDPOINT_OUT | usb1.TYPE_VENDOR | usb1.RECIPIENT_INTERFACE
  REQUEST_PUSH_FRAME = 1
  # Write frame of all red LEDs
  handle.controlWrite(VENDOR_OUT, REQUEST_PUSH_FRAME, 0, 0, bytes(frame))
  # Clear display by writing 'black' frame
  handle.controlWrite(VENDOR_OUT, REQUEST_PUSH_FRAME, 0, 0, bytes([0]*led_count*4))

  handle.close()
