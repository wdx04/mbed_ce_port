add_library(pulseout INTERFACE)
target_sources(pulseout INTERFACE PulseOut.cpp)
target_include_directories(pulseout INTERFACE .)
