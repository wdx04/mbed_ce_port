add_library(st7789-spi INTERFACE)
target_sources(st7789-spi INTERFACE
    ST7789_SPI.cpp)
target_include_directories(st7789-spi INTERFACE .)
target_link_libraries(st7789-spi INTERFACE cvcore)
