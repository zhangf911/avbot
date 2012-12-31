find_package(Boost 1.46 REQUIRED COMPONENTS date_time regex signals thread)
find_package(OpenSSL REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${LIBXML2_INCLUDE_DIR})
include_directories(iksemel/include)

add_subdirectory(iksemel)

add_library(xmpp STATIC xmpp.cpp)

target_link_libraries(xmpp ${OPENSSL_LIBRARIES})
target_link_libraries(xmpp ${Boost_LIBRARIES} -pthread)
target_link_libraries(xmpp iksemel)

add_executable(test test.cpp)

target_link_libraries(test xmpp)
