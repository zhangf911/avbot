find_package(Boost 1.46 REQUIRED COMPONENTS date_time regex signals thread)

include_directories(${Boost_INCLUDE_DIRS})

add_library(xmpp STATIC xmpp.cpp)

target_link_libraries(xmpp ${Boost_LIBRARIES} -lgloox -pthread)

add_executable(test test.cpp)

target_link_libraries(test xmpp)
