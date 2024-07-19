add_library(mxchip INTERFACE)
target_sources(mxchip INTERFACE ATParser.cpp MXCHIP.cpp MXCHIPInterface.cpp)
target_include_directories(mxchip INTERFACE .)
