add_library(Net)

project(kylea)

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

message("OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message("OpenSSL libraries: ${OPENSSL_LIBRARIES}")

#include_directories(${OPENSSL_INCLUDE_DIR})
#list(APPEND LIB_LIST ${OPENSSL_LIBRARIES})

target_sources(Net PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MetaMessage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Session.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SessionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ClientHTTPSession.cpp)

target_include_directories(Net PUBLIC include)

target_link_libraries(Net PUBLIC ${Boost_LIBRARIES})
target_link_libraries(Net PUBLIC ${OPENSSL_LIBRARIES})
target_link_libraries(Net PUBLIC buffer_provider Misc)
#target_link_libraries(Net PUBLIC Misc)