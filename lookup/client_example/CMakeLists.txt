cmake_minimum_required (VERSION 2.8)
project (EV_LOOKUP_CLIENT_EXAMPLE)
include_directories(${HLV_LOOKUP_CLIENT_LIB_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
file(GLOB lookup_client_sources . src/*.cc)
add_executable(lookup_client_mk1 ${lookup_client_sources})
target_link_libraries(lookup_client_mk1 ${Boost_LIBRARIES})
target_link_libraries(lookup_client_mk1 lookup_client) 

