cmake_minimum_required (VERSION 2.8)
project (HLV_LOOKUP_CLIENT_LIB)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
PROTOBUF_GENERATE_CPP(HLV_PROTO_LKP_SRC HLV_PROTO_LKP_HDRS ${EV_LOOKUP_SOURCE_DIR}/../proto/lookup.proto)
file(GLOB lookup_client_sources . src/*.cc)
add_library(lookup_client SHARED ${HLV_PROTO_LKP_SRC} 
                ${HLV_PROTO_LKP_HDRS} ${lookup_client_sources})
target_link_libraries(lookup_client ${PROTOBUF_LIBRARIES})
target_link_libraries(lookup_client ${Boost_LIBRARIES})
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package (Threads)
    target_link_libraries(lookup_client ${CMAKE_THREAD_LIBS_INIT})
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

