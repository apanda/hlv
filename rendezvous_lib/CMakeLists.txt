cmake_minimum_required (VERSION 2.8)
project (EV_RENDEZVOUS_LIB)
include_directories(${EV_RENDEZVOUS_LIB_SOURCE_DIR}/include)
include_directories(${HLV_LOOKUP_CLIENT_LIB_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
PROTOBUF_GENERATE_CPP(EV_RENDEZVOUS_SRC EV_RENDEZVOUS_HDRS ${EV_LOOKUP_SOURCE_DIR}/proto/ebox.proto)
file(GLOB simple_sources . src/*.cc)
add_library(ev_rendezvous_lib SHARED ${simple_sources}
                                     ${EV_RENDEZVOUS_SRC}
                                     ${EV_RENDEZVOUS_HDRS})
target_link_libraries(ev_rendezvous_lib ${Boost_LIBRARIES})
target_link_libraries(ev_rendezvous_lib ${PROTOBUF_LIBRARIES})
target_link_libraries(ev_rendezvous_lib lookup_client) 
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package (Threads)
    target_link_libraries(ev_rendezvous_lib ${CMAKE_THREAD_LIBS_INIT})
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

