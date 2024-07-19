add_library(textlcd INTERFACE)
target_sources(textlcd INTERFACE TextLCD.cpp)
target_include_directories(textlcd INTERFACE .)
