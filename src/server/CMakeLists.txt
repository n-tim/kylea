add_executable(server)

project(kylea)

target_sources(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(server Net proto)
install(TARGETS server DESTINATION bin)

add_custom_command(
TARGET server POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/cert.pem
        ${CMAKE_CURRENT_BINARY_DIR}/cert.pem)

    add_custom_command(
        TARGET server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/key.pem
                ${CMAKE_CURRENT_BINARY_DIR}/key.pem)
