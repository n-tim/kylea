cmake_minimum_required(VERSION 3.11)

project(kylea LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.70.0 REQUIRED system thread)

add_subdirectory(proto)
add_subdirectory(src)
#add_subdirectory(test)