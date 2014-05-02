cmake_minimum_required(VERSION 2.6)
project(qqwry)

include_directories(include)

add_executable(getip test/test.cpp)

find_package(ZLIB 1.2)

if (ZLIB_FOUND)
add_definitions(-DHAVE_ZLIB)
target_link_libraries(getip ${ZLIB_LIBRARIES})
else()
#add_library(miniz STATIC miniz.c)
#target_link_libraries(getip miniz)
endif()

