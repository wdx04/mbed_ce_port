add_library(ds1302 INTERFACE)
target_sources(ds1302 INTERFACE DS1302.cpp)
target_include_directories(ds1302 INTERFACE .)
