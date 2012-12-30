
add_library(xmpp STATIC xmpp.cpp)

add_executable(test test.cpp)
target_link_libraries(test xmpp)