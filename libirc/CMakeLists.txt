cmake_minimum_required(VERSION 2.8)
project(libirc)

find_package(Boost 1.48 REQUIRED COMPONENTS date_time system)

include_directories(${Boost_INCLUDE_DIRS})

add_library(libirc STATIC irc.cpp)

target_link_libraries(libirc ${Boost_LIBRARIES})