cmake_minimum_required(VERSION 2.6)
project(avbot)
#SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
option(ENABLE_STATIC "link to boost staticaly" ON)
option(ENABLE_TEST "build xmpp/pop3 test" OFF)
option(ENABLE_CXX11 "build avbot with cxx11" OFF)
option(ENABLE_LTO "build avbot with LTO" OFF)

if(ENABLE_STATIC)
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_STATIC_RUNTIME    ON)
endif()

if(ENABLE_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()
if(ENABLE_LTO)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
	set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -flto -Wl,-flto")
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

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR})
include_directories(avproxy/include)

add_subdirectory(libwebqq)
add_subdirectory(libirc)
add_subdirectory(libxmpp)
add_subdirectory(libpop3)

add_definitions(-DQQBOT_VERSION=\"3.3\")
add_executable(avbot main.cpp)

target_link_libraries(avbot webqq irc xmpp pop3)
target_link_libraries(avbot ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS avbot RUNTIME DESTINATION bin)

if( NOT Boost_USE_STATIC_RUNTIME )
	message(STATUS "dynamic link to boost is not recomanded")
endif()

