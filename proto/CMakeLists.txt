add_library(proto)

project(kylea)

include(FindProtobuf)
find_package(Protobuf REQUIRED)

PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER auth.proto)

execute_process(COMMAND cp ${PROTO_HEADER} ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_sources(proto PRIVATE ${PROTO_SRC})

target_link_libraries(proto PRIVATE ${PROTOBUF_LIBRARY})