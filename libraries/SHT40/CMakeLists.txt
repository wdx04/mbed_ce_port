add_library(sht40 INTERFACE)
target_sources(sht40 INTERFACE SHT40.cpp)
target_include_directories(sht40 INTERFACE .)
