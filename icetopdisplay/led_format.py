import numpy

class FormatAPA102:
  "APA102 data format: 5b global brightness, 3×8b RGB"
  MAX_BRIGHTNESS = 2**5-1

  @classmethod
  def float_to_led_data(cls, rgb):
    # Factor out brightness from colour
    brightness = max(1/cls.MAX_BRIGHTNESS, max(rgb))
    scaling = brightness / cls.MAX_BRIGHTNESS
    rgb = [int(round(255 * c/brightness)) for c in numpy.power(rgb, 2.2)]
    return [int(round(brightness*cls.MAX_BRIGHTNESS)), rgb[0], rgb[1], rgb[2]]

class FormatAPA102Comp:
  "APA102 data format with constant (maximum) brightness, to emulate WS2812 LEDs."
  @classmethod
  def float_to_led_data(cls, rgb):
    rgb = [int(round(255*c)) for c in numpy.power(rgb, 2.2)]
    return [0x1F, rgb[0], rgb[1], rgb[2]]
