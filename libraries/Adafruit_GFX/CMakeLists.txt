add_library(adafruit-gfx INTERFACE)
target_sources(adafruit-gfx INTERFACE
    Adafruit_GFX.cpp
    Adafruit_SSD1306_SpiFont.cpp
    Adafruit_SSD1306.cpp
    Adafruit_UC1601S.cpp)
target_include_directories(adafruit-gfx INTERFACE .)
