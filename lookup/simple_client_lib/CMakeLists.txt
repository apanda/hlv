cmake_minimum_required (VERSION 2.8)
project (EV_SIMPLE_CLIENT_LIB)
include_directories(${EV_SIMPLE_CLIENT_LIB_SOURCE_DIR}/include)
include_directories(${HLV_LOOKUP_CLIENT_LIB_SOURCE_DIR}/include)
file(GLOB simple_sources . src/*.cc)
add_library(ev_sclient SHARED ${simple_sources})
target_link_libraries(ev_sclient ${Boost_LIBRARIES})

target_link_libraries(ev_sclient lookup_client) 
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package (Threads)
    target_link_libraries(ev_sclient ${CMAKE_THREAD_LIBS_INIT})
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

