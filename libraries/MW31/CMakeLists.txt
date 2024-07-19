add_library(mw31 INTERFACE)
target_sources(mw31 INTERFACE ATParser.cpp MW31.cpp MW31Interface.cpp)
target_include_directories(mw31 INTERFACE .)
target_link_libraries(mw31 INTERFACE mbed-lwipstack)