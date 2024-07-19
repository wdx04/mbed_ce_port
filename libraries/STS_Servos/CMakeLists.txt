add_library(sts-servos INTERFACE)
target_sources(sts-servos INTERFACE STSServoDriver.cpp)
target_include_directories(sts-servos INTERFACE .)
