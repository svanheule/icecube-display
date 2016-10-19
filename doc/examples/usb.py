#!/usr/bin/python3
import usb.core
import usb.util
import struct

VENDOR_IN = usb.util.build_request_type(
    usb.util.CTRL_IN
  , usb.util.CTRL_TYPE_VENDOR
  , usb.util.CTRL_RECIPIENT_DEVICE
)

for device in usb.core.find(idVendor=0x1CE3, find_all=True):
  device.set_configuration()
  print("Querying device: {}".format(device.serial_number))

  try:
    data = device.ctrl_transfer(
        VENDOR_IN
      , 2 # Vendor request DISPLAY_PROPERTIES
      , 0
      , 0
      , 2 # Read only properties header: properties length
      , 5000
    )

    size, = struct.unpack("<H", data)

    data = device.ctrl_transfer(
        VENDOR_IN
      , 2 # Vendor request DISPLAY_PROPERTIES
      , 0
      , 0
      , size
      , 5000
    )

    tlv_fields = parseTlvData(data[2:])

    [...]

  except usb.core.USBError as e:
    print("Error reading information: {}".format(e))
