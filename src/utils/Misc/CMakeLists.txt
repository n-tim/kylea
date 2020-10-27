add_library(Misc)

project(kylea)

target_sources(Misc PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/Timer.cpp)
target_include_directories(Misc PUBLIC include)