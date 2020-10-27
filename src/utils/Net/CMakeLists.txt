add_library(Net)

project(kylea)

target_sources(Net PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MetaMessage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Session.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SessionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Client.cpp)

target_include_directories(Net PUBLIC include)

target_link_libraries(Net PUBLIC ${Boost_LIBRARIES})
target_link_libraries(Net PUBLIC buffer_provider)
target_link_libraries(Net PUBLIC Misc)