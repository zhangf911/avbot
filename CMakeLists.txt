cmake_minimum_required(VERSION 2.8)
project(qqbot)

#SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_STATIC_RUNTIME    ON)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
find_package(Boost 1.46 REQUIRED COMPONENTS date_time filesystem system program_options regex)

find_package(Threads)

add_subdirectory(libwebqq)
add_subdirectory(libirc)
add_subdirectory(libxmpp)

include_directories(${Boost_INCLUDE_DIRS})

add_executable(qqbot main.cpp)

target_link_libraries(qqbot libwebqq libirc)
target_link_libraries(qqbot ${Boost_LIBRARIES} -pthread)

install(TARGETS qqbot RUNTIME DESTINATION bin)