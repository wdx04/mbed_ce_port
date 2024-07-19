add_library(jpegdec INTERFACE)
target_sources(jpegdec INTERFACE src/JPEGDEC.cpp src/jpeg.inl)
target_include_directories(jpegdec INTERFACE src)
