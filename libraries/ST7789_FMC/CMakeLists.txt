add_library(st7789-fmc INTERFACE)
target_sources(st7789-fmc INTERFACE
    ST7789_FMC_8Bit.cpp
    FMCTransport_8Bit.cpp)
target_include_directories(st7789-fmc INTERFACE .)
target_link_libraries(st7789-fmc INTERFACE cvcore)
