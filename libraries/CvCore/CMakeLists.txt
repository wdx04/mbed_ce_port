add_library(cvcore INTERFACE)
target_sources(cvcore INTERFACE
    cvcore.cpp
    cvfonts.cpp
    cvgui.cpp
    cvimgproc.cpp
    default_ascii_font.c
    default_gb2312_font.c
    dmaops.cpp)
target_include_directories(cvcore INTERFACE .)
