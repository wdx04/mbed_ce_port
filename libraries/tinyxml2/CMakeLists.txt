add_library(tinyxml2 INTERFACE)
target_sources(tinyxml2 INTERFACE tinyxml2.cpp)
target_include_directories(tinyxml2 INTERFACE .)
