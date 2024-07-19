add_library(st7735-spi INTERFACE)
target_sources(st7735-spi INTERFACE
    ST7735_SPI.cpp)
target_include_directories(st7735-spi INTERFACE .)
target_link_libraries(st7735-spi INTERFACE cvcore)
