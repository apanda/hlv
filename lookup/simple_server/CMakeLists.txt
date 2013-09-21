cmake_minimum_required (VERSION 2.8)
project (SIMPLE_SERVER)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings")
include_directories(${CMAKE_CURRENT_BINARY_DIR})
PROTOBUF_GENERATE_CPP(HLV_PROTO_SRC HLV_PROTO_HDRS ${EV_LOOKUP_SOURCE_DIR}/../proto/service.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${EV_MISC_SOURCE_DIR}/include)
include_directories(${EV_SIMPLE_SERVER_SOURCE_DIR}/include)
include_directories(${EV_LOOKUP_UPDATE_LIB_SOURCE_DIR}/include)
file(GLOB dropbox_client_sources . src/*.cc)
file(GLOB dropbox_client_csources . src/*.c)
file(GLOB hlv_common_sources . ../../common/src/*.cc)
add_executable(simple_server ${dropbox_client_sources} 
                                  ${dropbox_client_csources} 
                                  ${hlv_common_sources}
                                  ${HLV_PROTO_SRC}
                                  ${HLV_PROTO_HDRS})
target_link_libraries(simple_server ${Boost_LIBRARIES})
target_link_libraries(simple_server  ev_misc)
target_link_libraries(simple_server ev_simple)
target_link_libraries(simple_server update_client)
