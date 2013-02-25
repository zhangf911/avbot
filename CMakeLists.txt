cmake_minimum_required(VERSION 2.6)
project(qqbot)
#SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
option(ENABLE_STATIC "link to boost staticaly" ON)
option(ENABLE_TEST "build xmpp test" OFF)

if(ENABLE_STATIC)
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_STATIC_RUNTIME    ON)
endif()

find_package(Boost 1.48 COMPONENTS date_time filesystem system program_options regex)

if(NOT Boost_FOUND)
	set(Boost_USE_STATIC_LIBS        OFF)
	set(Boost_USE_STATIC_RUNTIME    OFF)
	find_package(Boost 1.48 REQUIRED COMPONENTS date_time filesystem system program_options regex)
	if(NOT Boost_FOUND )
		message(FATAL_ERROR "need boost >= 1.48")
	endif()
endif()

find_package(Threads)

add_subdirectory(libwebqq)
add_subdirectory(libirc)
add_subdirectory(libxmpp)

include_directories(${Boost_INCLUDE_DIRS})

add_definitions(-DQQBOT_VERSION=\"2.2\")
add_executable(qqbot main.cpp)

target_link_libraries(qqbot webqq irc xmpp)
target_link_libraries(qqbot ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS qqbot RUNTIME DESTINATION bin)

if( NOT Boost_USE_STATIC_RUNTIME )
	message(STATUS "dynamic link to boost is not recomanded")
endif()
