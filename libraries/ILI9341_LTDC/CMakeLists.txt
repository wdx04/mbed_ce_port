add_library(ili9341-ltdc INTERFACE)
target_sources(ili9341-ltdc INTERFACE
    ILI9341_LTDC.cpp)
target_include_directories(ili9341-ltdc INTERFACE .)
target_link_libraries(ili9341-ltdc INTERFACE cvcore)
