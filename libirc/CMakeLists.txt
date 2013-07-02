cmake_minimum_required(VERSION 2.6)
project(libirc)

find_package(Boost 1.54 REQUIRED COMPONENTS date_time system)

include_directories(${Boost_INCLUDE_DIRS})

if( ENABLE_TEST )
add_definitions(-DDEBUG)
add_executable(irc3test test.cpp)
target_link_libraries(irc3test ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )

endif()