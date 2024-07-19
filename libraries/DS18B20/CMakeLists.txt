add_library(ds18b20 INTERFACE)
target_sources(ds18b20 INTERFACE ds18b20.cpp)
target_include_directories(ds18b20 INTERFACE .)
