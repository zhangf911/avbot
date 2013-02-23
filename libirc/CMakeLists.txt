cmake_minimum_required(VERSION 2.6)
project(libirc)

find_package(Boost 1.48 REQUIRED COMPONENTS date_time system)

include_directories(${Boost_INCLUDE_DIRS})

add_library(irc STATIC irc.cpp)

target_link_libraries(irc ${Boost_LIBRARIES})
