add_library(ili9341-spi INTERFACE)
target_sources(ili9341-spi INTERFACE
    ILI9341_SPI.cpp)
target_include_directories(ili9341-spi INTERFACE .)
target_link_libraries(ili9341-spi INTERFACE cvcore)
