cmake_minimum_required (VERSION 2.8)
project (ECHO_SERVER)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings")
include_directories(${CMAKE_CURRENT_BINARY_DIR})
PROTOBUF_GENERATE_CPP(HLV_PROTO_SRC HLV_PROTO_HDRS ${EV_LOOKUP_SOURCE_DIR}/proto/service.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${EV_MISC_SOURCE_DIR}/include)
include_directories(${ECHO_SERVER_LIB_SOURCE_DIR}/include)
include_directories(${EV_LOOKUP_UPDATE_LIB_SOURCE_DIR}/include)
file(GLOB echo_server_sources . src/*.cc)
file(GLOB echo_server_csources . src/*.c)
file(GLOB hlv_common_sources . ../common/src/*.cc)
add_executable(echo_server ${echo_server_sources} 
                                  ${echo_server_csources} 
                                  ${hlv_common_sources}
                                  ${HLV_PROTO_SRC}
                                  ${HLV_PROTO_HDRS})
target_link_libraries(echo_server ${Boost_LIBRARIES})
target_link_libraries(echo_server  ev_misc)
target_link_libraries(echo_server echo_lib)
target_link_libraries(echo_server update_client)
