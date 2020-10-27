add_executable(client)

project(kylea)

target_sources(client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(client Net)
install(TARGETS client DESTINATION bin)
