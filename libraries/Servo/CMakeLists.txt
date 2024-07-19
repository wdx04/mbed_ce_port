add_library(servo INTERFACE)
target_sources(servo INTERFACE Servo.cpp)
target_include_directories(servo INTERFACE .)
