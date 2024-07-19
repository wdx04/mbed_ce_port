add_library(audioplayer INTERFACE)
target_sources(audioplayer INTERFACE AudioPlayer.cpp WaveAudioStream.cpp)
target_include_directories(audioplayer INTERFACE .)
