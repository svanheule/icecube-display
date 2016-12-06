from .geometry import station_to_pixel, pixel_to_coordinates, LED_COUNT


class TextDisplay:
  _FG_COLOR_TEMPLATE = "\033[38;2;{};{};{}m" # RGB foreground template
  _BG_COLOR = "\033[48;2;0;0;0m" # Black
  _COLOR_END = "\033[0m" # Reset colors

  def print_frame(self, data, return_to_start=False):
    def transform(position):
      x = 2*position[0] - position[1] + 5
      y = position[1]
      return (x,y)

    line = 0
    column = 0
    station = 1
    while station < LED_COUNT+1:
      pixel = station_to_pixel(station)
      x, y = transform(pixel_to_coordinates(pixel))
        # RGB foreground, white background (one char padding)
      if x == column:
        # See https://en.wikipedia.org/wiki/ANSI_escape_code#graphics
        rgb = [int(255*c) for c in data[pixel]]
        print(self._FG_COLOR_TEMPLATE.format(*rgb)+"0", end="")
        station += 1
      elif x == column+1:
        # Set background color
        print(self._BG_COLOR+" ", end="")
      else:
        print(" ", end="")

      if y != line:
        print(self._COLOR_END)
        line += 1
        column = 0
      else:
        column += 1

    print(" "+self._COLOR_END)

    if return_to_start:
      print("\033[{}A".format(line), end='\r')
