cmake_minimum_required (VERSION 2.8)
project (EV_RENDEZVOUS_EDGE)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${EV_MISC_SOURCE_DIR}/include)
include_directories(${EV_LOOKUP_UPDATE_LIB_SOURCE_DIR}/include)
PROTOBUF_GENERATE_CPP(HLV_PROTO_LKP_SRC HLV_PROTO_LKP_HDRS ${EV_LOOKUP_SOURCE_DIR}/proto/ebox.proto)

file(GLOB rendezvous_sources . src/*.cc)
add_executable(rendezvous  
    ${HLV_PROTO_LKP_SRC} ${HLV_PROTO_LKP_HDRS} ${rendezvous_sources})
target_link_libraries(rendezvous ${PROTOBUF_LIBRARIES})
target_link_libraries(rendezvous ${Boost_LIBRARIES})
target_link_libraries(rendezvous  ev_misc)
target_link_libraries(rendezvous update_client)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package (Threads)
    target_link_libraries(rendezvous ${CMAKE_THREAD_LIBS_INIT})
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

