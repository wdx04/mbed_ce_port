## CMakeLists which sets up the custom targets in this project.

# To create a custom target, you first need to figure out the CMake name of the target
# from its Mbed name.  This name is calculated as:
#
# 1. Convert target to lowercase
# 2. Replace all underscores (_) with hyphens (-)
# 3. add "mbed-" prefix.
#
# So, the target named 'L452RE_SIMPLE' will be called mbed-l452re-simple in CMake.
# The Mbed build system will look for a target of this name when configuring, so make sure to create it.

# Another thing to keep in mind is how the Mbed targets are organized for STM32L452.
# At the lowest level, we have mbed-nucleo-l452re-p, which is defined in
# mbed-os/targets/TARGET_STM/TARGET_STM32L4/TARGET_STM32L452xE/TARGET_NUCLEO_L452RE_P/CMakeLists.txt.
# This target is for the dev board specifically, and contains the pinmap files and nothing else.
#
# Up from there, we have mbed-stm32l452xe, which covers any and all boards with the STM32L452 chip.
# This includes the clocking config (system_clock.c), the linker script, and other chip-specific stuff.
# This target is defined in mbed-os/targets/TARGET_STM/TARGET_STM32L4/TARGET_STM32L452xE/CMakeLists.txt.
# By declaring mbed-stm32l452xe as an INTERFACE dependency, mbed-nucleo-l452re-p pulls in all of these aforementioned
# files.
#
# And going up one more level, we have mbed-stm32l4.  This target is for the entire STM32L4 family of chips
# (which are largely compatible from a software perspective).  It includes the actual HAL code used to drive
# the chip.
#
# Note: This is an example for STM32; other vendors can and do set up this hierarchy differently.
# In particular, it's a crapshoot whether the clock config file and the linker script will be
# part of the dev board target or with the MCU target.  Check the manufacturer files to be sure!

add_library(mbed-stm32g030c8 INTERFACE)
target_sources(mbed-stm32g030c8 INTERFACE TARGET_STM32G030C8/PeripheralPins.c TARGET_STM32G030C8/system_clock.c)
target_include_directories(mbed-stm32g030c8 INTERFACE TARGET_STM32G030C8)
target_link_libraries(mbed-stm32g030c8 INTERFACE mbed-stm32g030x8)

add_library(mbed-stm32f405rg INTERFACE)
target_sources(mbed-stm32f405rg INTERFACE TARGET_STM32F405RG/PeripheralPins.c TARGET_STM32F405RG/system_clock.c)
target_include_directories(mbed-stm32f405rg INTERFACE TARGET_STM32F405RG)
target_link_libraries(mbed-stm32f405rg INTERFACE mbed-stm32f405xg)

add_library(mbed-stm32u575ri INTERFACE)
target_sources(mbed-stm32u575ri INTERFACE
    TARGET_STM32U575RI/PeripheralPins.c
    TARGET_STM32U575RI/bsp_ospi_w25q128.c
    TARGET_STM32U575RI/octospi.c
    TARGET_STM32U575RI/system_clock_12M.c
    TARGET_STM32U575RI/targetbsp.c
)
target_include_directories(mbed-stm32u575ri INTERFACE TARGET_STM32U575RI)
target_link_libraries(mbed-stm32u575ri INTERFACE mbed-stm32u575xi)

add_library(mbed-stm32h7b0vb INTERFACE)
target_sources(mbed-stm32h7b0vb INTERFACE
    TARGET_STM32H7B0VB/system_clock.c
    TARGET_STM32H7B0VB/PeripheralPins.c
)
target_include_directories(mbed-stm32h7b0vb INTERFACE TARGET_STM32H7B0VB)
target_link_libraries(mbed-stm32h7b0vb INTERFACE mbed-stm32h7b3xiq mbed-storage-blockdevice)

add_library(mbed-stm32h750xb-jk INTERFACE)
    target_sources(mbed-stm32h750xb-jk INTERFACE
    TARGET_STM32H750XB_JK/PeripheralPins.c
    TARGET_STM32H750XB_JK/TargetInit.c
    TARGET_STM32H750XB_JK/system_clock_hse.c
)
target_include_directories(mbed-stm32h750xb-jk INTERFACE TARGET_STM32H750XB_JK)
target_link_libraries(mbed-stm32h750xb-jk INTERFACE mbed-stm32h750xb mbed-storage-blockdevice)

add_library(mbed-stm32h750b-dk INTERFACE)
target_sources(mbed-stm32h750b-dk INTERFACE
    TARGET_STM32H750B_DK/PeripheralPins.c
    TARGET_STM32H750B_DK/TargetInit.c
)
target_include_directories(mbed-stm32h750b-dk INTERFACE TARGET_STM32H750B_DK)
target_link_libraries(mbed-stm32h750b-dk INTERFACE mbed-stm32h750xb mbed-storage-blockdevice)

add_library(mbed-iot-devkit-stm32l496vg INTERFACE)
target_sources(mbed-iot-devkit-stm32l496vg INTERFACE
    TARGET_IOT_DEVKIT_STM32L496VG/PeripheralPins.c
)
target_include_directories(mbed-iot-devkit-stm32l496vg INTERFACE TARGET_IOT_DEVKIT_STM32L496VG)
target_link_libraries(mbed-iot-devkit-stm32l496vg INTERFACE mbed-stm32l496xg mbed-storage-blockdevice)

add_library(mbed-stm32f407ve-jz INTERFACE)
target_sources(mbed-stm32f407ve-jz INTERFACE
    TARGET_STM32F407VE_JZ/PeripheralPins.c
    TARGET_STM32F407VE_JZ/stm32f4_eth_conf.c
    TARGET_STM32F407VE_JZ/stm32f4_eth_init.c
    TARGET_STM32F407VE_JZ/system_clock_25m.c
)
target_include_directories(mbed-stm32f407ve-jz INTERFACE TARGET_STM32F407VE_JZ)
target_link_libraries(mbed-stm32f407ve-jz INTERFACE mbed-stm32f407xe)
