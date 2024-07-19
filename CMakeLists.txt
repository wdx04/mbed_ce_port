cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.19)

# Initialize Mbed OS build system. 
# Note: This block must be before the include of app.cmake
set(MBED_APP_JSON_PATH mbed_app.json5)
set(CUSTOM_TARGETS_JSON_PATH custom_targets/custom_targets.json5) 
include(mbed-os/tools/cmake/app.cmake)

set(MBED_RESET_BAUDRATE 115200)
set(MBED_UPLOAD_ENABLED TRUE)
set(PICOTOOL_UPLOAD_ENABLED TRUE)
set(STM32CUBE_UPLOAD_ENABLED TRUE)
if("STM32H750XB_JK" IN_LIST MBED_TARGET_LABELS)
    set(STM32CUBE_EXTERNAL_LOADERS -el ${CMAKE_SOURCE_DIR}/ext_loaders/STM32H750XB-JK-QSPIF.stldr)
else()
    set(STM32CUBE_EXTERNAL_LOADERS)
endif()
set(STM32CUBE_CONNECT_COMMAND ${STM32CUBE_EXTERNAL_LOADERS} -c port=SWD mode=UR)
set(STM32CUBE_GDBSERVER_ARGS ${STM32CUBE_EXTERNAL_LOADERS} -d -k)

add_subdirectory(custom_targets)
add_subdirectory(mbed-os)
add_subdirectory(libraries)

project(mbed-app)

# add subdirectories and build targets here
add_executable(mbed-app main.cpp)
target_link_libraries(mbed-app mbed-os) # Can also link to mbed-baremetal here


mbed_set_post_build(mbed-app)

mbed_finalize_build()
