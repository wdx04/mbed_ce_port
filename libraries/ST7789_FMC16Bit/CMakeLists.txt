add_library(st7789-fmc-16bit INTERFACE)
target_sources(st7789-fmc-16bit INTERFACE
    ST7789_FMC_16Bit.cpp
    FMCTransport_16Bit.cpp)
target_include_directories(st7789-fmc-16bit INTERFACE .)
target_link_libraries(st7789-fmc-16bit INTERFACE cvcore)
