import numpy

class LedFormat:
  MAX_BRIGHTNESS = 2**5-1

  @classmethod
  def float_to_led_data(cls, rgb):
    # Factor out brightness from colour
    brightness = max(1/cls.MAX_BRIGHTNESS, max(rgb))
    scaling = brightness / cls.MAX_BRIGHTNESS
    rgb = [int(round(255 * c/brightness)) for c in numpy.power(rgb, 2.2)]
    return [int(round(brightness*cls.MAX_BRIGHTNESS)), rgb[0], rgb[1], rgb[2]]
