add_library(lis3dsh INTERFACE)
target_sources(lis3dsh INTERFACE LIS3DSH.cpp)
target_include_directories(lis3dsh INTERFACE .)
