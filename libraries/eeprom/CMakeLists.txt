add_library(eeprom INTERFACE)
target_sources(eeprom INTERFACE eeprom.cpp)
target_include_directories(eeprom INTERFACE .)
