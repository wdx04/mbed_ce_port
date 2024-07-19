add_library(necir INTERFACE)
target_sources(necir INTERFACE necir.cpp)
target_include_directories(necir INTERFACE .)
target_link_libraries(necir INTERFACE stm32corelib)