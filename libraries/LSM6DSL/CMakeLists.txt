add_library(lsm6dsl INTERFACE)
target_sources(lsm6dsl INTERFACE LSM6DSL_acc_gyro_driver.c LSM6DSLSensor.cpp)
target_include_directories(lsm6dsl INTERFACE .)
