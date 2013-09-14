cmake_minimum_required (VERSION 2.8)
project (HIREDIS_ASIO_LIB)
include_directories(${HIREDIS_ASIO_LIB_SOURCE_DIR}/include)
file(GLOB hiredis_sources . src/*.cc)
add_library(hiredis_asio SHARED ${hiredis_sources})
target_link_libraries(hiredis_asio ${Boost_LIBRARIES})

