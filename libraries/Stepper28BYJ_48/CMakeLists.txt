add_library(stepper-28byj INTERFACE)
target_sources(stepper-28byj INTERFACE Stepper28BYJ_48.cpp)
target_include_directories(stepper-28byj INTERFACE .)
