add_library(bmp280 INTERFACE)
target_sources(bmp280 INTERFACE BMP280.cpp)
target_include_directories(bmp280 INTERFACE .)
