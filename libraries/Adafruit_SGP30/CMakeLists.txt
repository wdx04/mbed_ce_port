add_library(adafruit-sgp30 INTERFACE)
target_sources(adafruit-sgp30 INTERFACE Adafruit_SGP30.cpp)
target_include_directories(adafruit-sgp30 INTERFACE .)
