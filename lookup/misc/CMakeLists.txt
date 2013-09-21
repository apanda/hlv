cmake_minimum_required (VERSION 2.8)
project (EV_MISC)
include_directories(${EV_MISC_SOURCE_DIR}/include)
file(GLOB misc_sources . src/*.cc)
file(GLOB misc_c_sources . src/*.c)
add_library(ev_misc SHARED ${misc_sources} ${misc_c_sources})

