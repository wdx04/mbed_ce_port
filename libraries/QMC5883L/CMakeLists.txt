add_library(qmc5883l INTERFACE)
target_sources(qmc5883l INTERFACE QMC5883L.cpp)
target_include_directories(qmc5883l INTERFACE .)
