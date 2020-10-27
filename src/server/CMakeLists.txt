add_executable(server)

project(kylea)

target_sources(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(server Net)
install(TARGETS server DESTINATION bin)
