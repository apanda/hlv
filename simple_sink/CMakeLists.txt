cmake_minimum_required (VERSION 2.8)
project (SIMPLE_SINK)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings")

# Needed for auth
include_directories(${CMAKE_CURRENT_BINARY_DIR})
PROTOBUF_GENERATE_CPP(HLV_PROTO_SRC HLV_PROTO_HDRS ${EV_LOOKUP_SOURCE_DIR}/proto/service.proto)

include_directories(${EV_LDISCOVERY_LIB_SOURCE_DIR}/include)
include_directories(${HLV_LOOKUP_CLIENT_LIB_SOURCE_DIR}/include)
include_directories(${EV_MISC_SOURCE_DIR}/include)
include_directories(${EV_SIMPLE_SERVER_SOURCE_DIR}/include)
include_directories(${EV_SIMPLE_CLIENT_LIB_SOURCE_DIR}/include)
include_directories(${EV_RENDEZVOUS_LIB_SOURCE_DIR}/include)

file(GLOB simple_sink_sources . src/*.cc)
file(GLOB simple_sink_csources . src/*.c)
file(GLOB hlv_common_sources . ../common/src/*.cc)

add_executable(simple_sink_mk1 ${simple_sink_sources} 
                                  ${simple_sink_csources} 
                                  ${hlv_common_sources}
                                  ${HLV_PROTO_SRC}
                                  ${HLV_PROTO_HDRS})

target_link_libraries(simple_sink_mk1 ${Boost_LIBRARIES})
target_link_libraries(simple_sink_mk1 ldiscovery_lib) 
target_link_libraries(simple_sink_mk1 lookup_client) 
target_link_libraries(simple_sink_mk1  ev_misc)
target_link_libraries(simple_sink_mk1 ev_simple)
target_link_libraries(simple_sink_mk1 ev_sclient)
target_link_libraries(simple_sink_mk1 ev_rendezvous_lib)
