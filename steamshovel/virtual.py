import os
import cairo
import hashlib
from LedDisplay import DisplayController

class VirtualController(DisplayController):
    '''
    Virtual controller that emulates an LED display with WS2811 LEDs.
    The device has an (initially empty) EEPROM of 2048 bytes, which can
    be written and read.
    If the `DEBUG_FRAME_PATH` environment variable is set to a existing
    path, display frames written to the virtual device will be dumped
    in this directory.
    '''
    __EEPROM_SIZE = 2048
    __EEPROM_SEG_SN = slice(0, 0x20)
    __EEPROM_SEG_DEV = slice(0x20, 0x20+0x10)
    __VIRT_CONTROLLERS = None

    def __init__(self, serial, data_type, ranges, group=None):
        self.device = None
        self.__eeprom = bytearray(self.__EEPROM_SIZE)

        self.serial_number = serial
        self.__eeprom[self.__EEPROM_SEG_SN] = self.serial_number.encode('utf-16-le')

        self.data_type = data_type
        self.led_type = self.LED_TYPE_WS2811
        self.data_ranges = sorted(ranges, key=lambda r: r[0])

        self.group = None
        if group is not None:
            group_string = '+'.join(group)
            h = hashlib.md5(group_string.encode('utf-8'))
            self.group = bytes(h.digest())

        if self.data_type == self.DATA_TYPE_IC_STRING:
          self.__eeprom[0x60:0x60+0x10] = self.group
          start, stop = self.data_ranges[0]
          deepcore = int(any(start == 79 for start, _ in self.data_ranges))
          device_data = bytearray([self.led_type, 0, start, stop, deepcore, 1])
          string_count = sum([stop-start+1 for start, stop in self.data_ranges])
          for first_string in range(0,4):
            offsets = list(range(first_string, string_count, 4))
            offsets.insert(0, len(offsets))
            eeprom_offset = 0x30+9*first_string
            self.__eeprom[eeprom_offset:eeprom_offset+len(offsets)] = bytearray(offsets)
        elif self.data_type == self.DATA_TYPE_IT_STATION:
          start, stop = self.data_ranges[0]
          nstations = stop-start+1
          device_data = bytearray([nstations, self.led_type, 0])
        data_slice = slice(
          self.__EEPROM_SEG_DEV.start,
          self.__EEPROM_SEG_DEV.start+len(device_data)
        )
        self.__eeprom[data_slice] = device_data

        self.__frame_counter = 0
        self.__frame_path = None
        path = os.getenv('DEBUG_FRAME_PATH')
        if path is not None:
            if os.path.exists(path) and os.path.isdir(path):
                self.__frame_path = path
                for path in os.listdir(self.__frame_path):
                    os.remove(self.__frame_path + '/' + path)
            else:
                raise ValueError('DEBUG_FRAME_PATH is not a directory')

    @classmethod
    def findAll(cls):
        if cls.__VIRT_CONTROLLERS is None:
            def sn(t, i):
                return 'ICD-{:2s}-VIR-{:04d}'.format(t, i)
            g = [sn('IC', 1), sn('IC', 2), sn('IC', 3)]
            m = [sn('IC', 4), sn('IC', 5), sn('IC', 6)]
            ic = cls.DATA_TYPE_IC_STRING
            it = cls.DATA_TYPE_IT_STATION
            cls.__VIRT_CONTROLLERS = [
                  cls(sn('IC', 1), ic, [(1,30)], g)
                , cls(sn('IC', 2), ic, [(31,50)], g)
                , cls(sn('IC', 3), ic, [(51,78)], g)
                , cls(sn('IC', 4), ic, [(1,30)], m)
                , cls(sn('IC', 5), ic, [(31,50),(79,86)], m)
                , cls(sn('IC', 6), ic, [(51,78)], m)
                , cls(sn('IT', 1), it, [(1,78)])
            ]
        return cls.__VIRT_CONTROLLERS

    def readDisplayInfo(self):
        tlv_list = [
              (self.DP_TYPE_INFORMATION_TYPE, 1, bytearray([self.data_type]))
            , (self.DP_TYPE_LED_TYPE, 1, bytearray([self.led_type]))
        ]
        for data_range in self.data_ranges:
            tlv = (self.DP_TYPE_INFORMATION_RANGE, 2, bytearray(data_range))
            if data_range[0] == 79:
                tlv_list.insert(0, tlv)
            else:
                tlv_list.append(tlv)
        if self.group is not None:
            tlv_list.append((self.DP_TYPE_GROUP_ID, 16, self.group))
        return tlv_list

    def readEepromSegment(self, offset, length):
        if offset < self.__EEPROM_SIZE and (offset+length) <= self.__EEPROM_SIZE:
            return self.__eeprom[offset:offset+length]
        else:
            raise ValueError("data block out of range")

    def writeEepromSegment(self, offset, data):
        if offset < self.__EEPROM_SIZE and offset+len(data) <= self.__EEPROM_SIZE:
            self.__eeprom[offset:offset+len(data)] = bytes(data)
        else:
            raise ValueError("data block out of range")

    def transmitDisplayBuffer(self, data):
        if len(data) != self.buffer_length:
          raise ValueError("Invalid buffer length")
        if self.__frame_path is not None:
            pixels = self.buffer_length // 3

            string_length = 1
            if self.data_type == self.DATA_TYPE_IC_STRING:
                string_length = 60
            strings = pixels // string_length
            scale = 5
            sfc = cairo.ImageSurface(cairo.FORMAT_RGB24, scale*strings+1, scale*string_length+1)
            ctx = cairo.Context(sfc)
            # draw white background
            ctx.rectangle(0, 0, scale*strings+1, scale*string_length+1)
            ctx.set_source_rgb(1, 1, 1)
            ctx.fill()
            # set transformation matrix
            ctx.translate(1, 1)
            ctx.scale(scale, scale)

            for string in range(strings):
                for dom in range(string_length):
                    offset = (string*string_length + dom)*3
                    r = float(data[offset]) / 255.
                    g = float(data[offset+1]) / 255.
                    b = float(data[offset+2]) / 255.
                    ctx.rectangle(string, dom, 0.8, 0.8)
                    ctx.set_source_rgb(r, g, b)
                    ctx.fill()
            fn = "{}/{}_{}.png".format(
                  self.__frame_path
                , self.serial_number
                , self.__frame_counter
            )
            sfc.write_to_png(fn)
            self.__frame_counter += 1
