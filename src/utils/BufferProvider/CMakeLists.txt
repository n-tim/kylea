add_library(buffer_provider)

project(kylea)

target_sources(buffer_provider PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BufferProvider.cpp)

target_include_directories(buffer_provider PUBLIC include)
target_link_libraries(buffer_provider Misc)