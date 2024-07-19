add_library(xpt2046 INTERFACE)
target_sources(xpt2046 INTERFACE XPT2046.cpp)
target_include_directories(xpt2046 INTERFACE .)
