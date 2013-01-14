cmake_minimum_required(VERSION 2.8)
project(qqbot)
#SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_STATIC_RUNTIME    ON)

find_package(Boost 1.48 COMPONENTS date_time filesystem system program_options regex)

if(NOT Boost_FOUND)
	set(Boost_USE_STATIC_LIBS        OFF)
	set(Boost_USE_STATIC_RUNTIME    OFF)
	find_package(Boost 1.48 COMPONENTS date_time filesystem system program_options regex)
	if(NOT Boost_FOUND )
		message(FATAL_ERROR "need boost >= 1.48")
	endif()
endif()

find_package(Threads)

add_subdirectory(libwebqq)
add_subdirectory(libirc)
add_subdirectory(libxmpp)
#add_subdirectory(lisp)

include_directories(${Boost_INCLUDE_DIRS})

add_executable(qqbot main.cpp)

target_link_libraries(qqbot libwebqq libirc libxmpp)
target_link_libraries(qqbot ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS qqbot RUNTIME DESTINATION bin)

if( NOT Boost_USE_STATIC_RUNTIME )
	message(STATUS "dynamic link to boost is not recomanded")
endif()
