import numpy

LED_COUNT = 78


# Module internal functions
def _get_station_key(coordinates):
  return (int(coordinates[0]) << 4) | int(coordinates[1])

# Module internal variables
_ROW_START = [1, 7, 14, 22, 31, 41, 51, 60, 68, 75, 79]
_ROW_OFFSET = [0, 0, 0, 0, 0, 1, 2, 3, 4, 5]

_pixel_station_map = numpy.empty(LED_COUNT, dtype=int)
_pixel_coordinates = numpy.empty((LED_COUNT, 2), dtype=int)

for row in range(len(_ROW_START)-1):
  pixel_start = _ROW_START[row]-1
  pixel_end = _ROW_START[row+1]-1

  station = _ROW_START[row]
  offset = _ROW_OFFSET[row]-_ROW_START[row]

  for pixel in range(pixel_start, pixel_end):
    _pixel_station_map[pixel] = station
    _pixel_coordinates[pixel] = [station+offset, row]
    station += 1

_coordinates_pixel_map = {}
for i,location in enumerate(_pixel_coordinates):
  _coordinates_pixel_map[_get_station_key(location)] = i

_station_pixel_map = numpy.empty(LED_COUNT, dtype=int)
for pixel, station in enumerate(_pixel_station_map):
  _station_pixel_map[station-1] = pixel


# Public functions
def pixel_to_station(pixel):
  if pixel in range(LED_COUNT):
    return _pixel_station_map[pixel]
  else:
    raise ValueError("Invalid pixel value")

def pixel_to_coordinates(pixel):
  if pixel in range(LED_COUNT):
    return _pixel_coordinates[pixel]
  else:
    raise ValueError("Invalid pixel value")


def station_to_pixel(station):
  return _station_pixel_map[station-1]

def station_to_coordinates(station):
  return pixel_to_coordinates(station_to_pixel(station))


def coordinates_to_pixel(coordinates):
  "Lookup a pixel number for a given coordinate pair."
  if _get_station_key(location) in _coordinates_pixel_map:
    return _coordinates_pixel_map[_get_station_key(location)]
  else:
    raise ValueError("Invalid coordinates {}").format(coordinates)

def coordinates_to_station(coordinates):
  return pixel_to_station(coordinates_to_pixel(coordinates))

