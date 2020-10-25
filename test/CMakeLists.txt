cmake_minimum_required(VERSION 3.11)
project(kylea_tests)

add_subdirectory(lib/googletest)

include_directories(lib/googletest/googletest/include)

add_executable(kylea_tests)

target_sources(kylea_tests PRIVATE main.cpp)

target_link_libraries(kylea_tests buffer_provider gtest)
install(TARGETS kylea_tests DESTINATION bin)