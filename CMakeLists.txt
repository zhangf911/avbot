cmake_minimum_required(VERSION 2.6)
project(avbot)
#SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
option(ENABLE_STATIC "link to boost staticaly" ON)
option(ENABLE_TEST "build xmpp/pop3 test" OFF)
option(ENABLE_CXX11 "build avbot with cxx11" OFF)
option(ENABLE_LTO "build avbot with LTO" OFF)
option(ENABLE_LIBSTDCXX_STATIC "link to static libstdc++" OFF)
option(ENABLE_LIBGCC_STATIC "link to static libgcc" OFF)

if(WIN32)
add_definitions(-D_WIN32_WINNT=0x0501 -DWINVER=0x0501)
set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lstdc++ -Wl,--enable-auto-import,-lstdc++ -Wl,--disable-auto-import ")
endif()

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g0")

set(CMAKE_C_FLAGS_TEST "-Og -g")
set(CMAKE_CXX_FLAGS_TEST "-Og -g0")

if(CMAKE_BUILD_TYPE EQUAL "TEST")
set(ENABLE_STATIC ON)
endif()

if(ENABLE_STATIC)
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_STATIC_RUNTIME    ON)
endif()

if(ENABLE_LIBSTDCXX_STATIC)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ ")
endif()

if(ENABLE_LIBGCC_STATIC)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc ")
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

if(WIN32 AND Boost_USE_STATIC_LIBS)
add_definitions(-DBOOST_THREAD_USE_LIB)
add_definitions(-DBOOST_FILESYSTEM_STATIC_LINK)
add_definitions(-DBOOST_ALL_STATIC_LINK)
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR})
include_directories(avproxy/include)
include_directories(process/)

add_subdirectory(libwebqq)
if(ENABLE_TEST)
add_subdirectory(libirc)
endif()
add_subdirectory(libxmpp)
add_subdirectory(libmailexchange)

add_definitions(-DQQBOT_VERSION=\"3.6\")
add_executable(avbot main.cpp botctl.cpp messagegroup.cpp)

target_link_libraries(avbot webqq xmpp mailexchange)
target_link_libraries(avbot ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS avbot RUNTIME DESTINATION bin)

if( NOT Boost_USE_STATIC_RUNTIME )
	message(STATUS "dynamic link to boost is not recomanded")
endif()

if(WIN32)
	link_libraries(-liconv -lssl -lcrypt32)
endif()