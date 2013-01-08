find_package(Boost 1.48 REQUIRED COMPONENTS date_time regex signals system)
include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory(gloox)

include_directories(gloox)

add_library(libxmpp STATIC xmpp_impl.cpp xmpp.cpp)
target_link_libraries(libxmpp ${Boost_LIBRARIES} gloox -pthread)

add_executable(test test.cpp)
target_link_libraries(test libxmpp)
