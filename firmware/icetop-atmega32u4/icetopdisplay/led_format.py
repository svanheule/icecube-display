import numpy

class LedDataFormat:
  LENGTH = 0

  @classmethod
  def float_to_led_data(cls, rgb):
    "Convert a list-like object of 3 floats to an array containing the data for one LED."
    pass

class FormatAPA102(LedDataFormat):
  "APA102 data format: 5b global brightness, 3*8b RGB"
  MAX_BRIGHTNESS = 2**5-1
  LENGTH = 4

  @classmethod
  def float_to_led_data(cls, rgb):
    # Factor out brightness from colour
    brightness = max(1./cls.MAX_BRIGHTNESS, max(rgb))
    scaling = brightness / cls.MAX_BRIGHTNESS
    rgb = [int(round(255 * c/brightness)) for c in numpy.power(rgb, 2.2)]
    return [int(round(brightness*cls.MAX_BRIGHTNESS)), rgb[0], rgb[1], rgb[2]]

class FormatAPA102Comp(LedDataFormat):
  "APA102 data format with constant (maximum) brightness, to emulate WS2812 LEDs."
  LENGTH = 4

  @classmethod
  def float_to_led_data(cls, rgb):
    rgb = [int(round(255*c)) for c in numpy.power(rgb, 2.2)]
    return [0x1F, rgb[0], rgb[1], rgb[2]]

class FormatWS2811(LedDataFormat):
  "WS2811/WS2812(B) LEDs with 3*8b RGB."
  LENGTH = 3

  @classmethod
  def float_to_led_data(cls, rgb):
    return [int(round(255*c)) for c in numpy.power(rgb, 2.2)]
