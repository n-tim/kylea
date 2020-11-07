add_executable(http_client)

project(kylea)

target_sources(http_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(http_client Net)
install(TARGETS http_client DESTINATION bin)
