find_package(Boost 1.46 REQUIRED COMPONENTS date_time regex signals thread)
find_package(OpenSSL REQUIRED)
find_package(LibXml2 REQUIRED)


include_directories(${Boost_INCLUDE_DIRS})
include_directories(${LIBXML2_INCLUDE_DIR})

add_library(xmpp STATIC xmpp.cpp)

target_link_libraries(xmpp ${OPENSSL_LIBRARIES} ${LIBXML2_LIBRARIES} ${GTK2_LIBRARIES})
target_link_libraries(xmpp ${Boost_LIBRARIES})

add_executable(test test.cpp)

target_link_libraries(test xmpp)