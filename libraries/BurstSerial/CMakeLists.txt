add_library(burstserial INTERFACE)
target_sources(burstserial INTERFACE BurstSerial.cpp)
target_include_directories(burstserial INTERFACE .)
