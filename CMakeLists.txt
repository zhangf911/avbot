cmake_minimum_required(VERSION 2.6)
project(avbot)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

if (NOT MSVC)

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
	set(OBJCOPYFORMATENDIAN "x86-64")
	set(BINARY_ARCHITECTURE "i386")
else()
	set(OBJCOPYFORMATENDIAN "${CMAKE_SYSTEM_PROCESSOR}")
	set(BINARY_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
endif()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(OBJCOPYFORMATELF "elf64")
else()
	set(OBJCOPYFORMATELF "elf32")
endif()

set(OBJCOPYFORMAT "${OBJCOPYFORMATELF}-${OBJCOPYFORMATENDIAN}")

endif()

include(cxx11.cmake)
include(cpacksettings.cmake)
include(tweakvcflasgs.cmake)

#SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
option(ENABLE_STATIC "link to boost staticaly" ON)
option(ENABLE_TEST "build xmpp/pop3 test" OFF)
option(ENABLE_CXX11 "build avbot with cxx11" OFF)
option(ENABLE_LTO "build avbot with LTO" OFF)
option(ENABLE_LIBSTDCXX_STATIC "link to static libstdc++" OFF)
option(ENABLE_LIBGCC_STATIC "link to static libgcc" OFF)
option(WITH_ICONV "boost is build with iconv" ON)
option(WITH_ICU "boost is build with icu " OFF)

if(WIN32)
set(ENABLE_LIBGCC_STATIC ON)
set(ENABLE_LIBSTDCXX_STATIC ON)
set(ENABLE_STATIC ON)
endif()

if(WIN32)
add_definitions(-D_WIN32_WINNT=0x0501 -DWINVER=0x0501)
if(NOT MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -g0")
endif()

if(MSVC)
 if(CMAKE_CL_64)
	link_directories(${PROJECT_SOURCE_DIR}/openssl/lib64)
 else()
	link_directories(${PROJECT_SOURCE_DIR}/openssl/lib)
 endif()

else()
	link_directories(${PROJECT_SOURCE_DIR}/openssl/lib64)
	link_directories(${PROJECT_SOURCE_DIR}/openssl/lib)
endif()

 include_directories( ${PROJECT_SOURCE_DIR}/openssl/include)

 link_directories(${PROJECT_SOURCE_DIR}/libsformingw64)
endif()

if(NOT MSVC AND NOT APPLE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g0 -O3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g0 -O3")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Og -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og -g0")
endif()

if(CMAKE_BUILD_TYPE EQUAL "TEST")
set(ENABLE_STATIC ON)
endif()

if(ENABLE_STATIC)
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_STATIC_RUNTIME    ON)
else(ENABLE_STATIC)
ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)
endif()

if(ENABLE_LIBSTDCXX_STATIC OR WIN32)
if(MSVC)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MT ")
else()
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ ")
endif()

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_STATIC_RUNTIME    ON)
endif()

if(ENABLE_LIBGCC_STATIC OR WIN32)
if(NOT MSVC)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc ")
endif()
endif()

if(ENABLE_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()
if(ENABLE_LTO)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
	set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -flto -Wl,-flto")
endif()

find_package(Boost 1.55 REQUIRED COMPONENTS timer chrono log log_setup date_time filesystem system program_options regex locale)

if(MSVC)
# add_definitions( -DBOOST_ALL_NO_LIB )
link_directories(${Boost_LIBRARY_DIR})
endif()

#add_definitions(-DBOOST_LOG_NO_THREADS)
find_package(Threads)

if(WIN32)
add_definitions(-DBOOST_ALL_STATIC_LINK)
add_definitions(-DBOOST_THREAD_USE_LIB)
add_definitions(-DBOOST_FILESYSTEM_STATIC_LINK)
add_definitions(-DBOOST_ALL_STATIC_LINK)
add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-DNOMINMAX)
endif()

find_package(OpenSSL)

add_definitions(-DAVHTTP_ENABLE_OPENSSL -DAVHTTP_DISABLE_MULTI_DOWNLOAD -DDISABLE_LOGGER)

configure_file(avbot.rc.in ${PROJECT_BINARY_DIR}/avbot.rc)

add_subdirectory(sqlite)

set(SQLITE3_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/sqlite/include/")
set(SQLITE3_LIBRARIES sqlite3)

file(GLOB SOCI_CORE_SOURCES RELATIVE ${PROJECT_SOURCE_DIR} soci/src/core/*.cpp)

add_library(soci_core STATIC ${SOCI_CORE_SOURCES})

file(GLOB SOCI_SQLITE3_SOURCES RELATIVE ${PROJECT_SOURCE_DIR} soci/src/backends/sqlite3/*.cpp)
add_library(soci_sqlite3 STATIC ${SOCI_SQLITE3_SOURCES})

configure_file(soci/src/core/soci_backends_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/soci_backends_config.h)

if(NOT WIN32)
  set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/soci/src/cmake ${CMAKE_MODULE_PATH})
  set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/soci/src/cmake/modules ${CMAKE_MODULE_PATH})
  set(DL_FIND_QUIETLY TRUE)
  find_package(DL)
  if(DL_FOUND)
    include_directories(${DL_INCLUDE_DIR})
    target_link_libraries(soci_core ${DL_LIBRARY})
  endif()
endif()

add_definitions(-DSOCI_LIB_PREFIX="" -DSOCI_LIB_SUFFIX="")

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(soci/src/core)
include_directories(soci/src/backends/sqlite3)
include_directories(${SQLITE3_INCLUDE_DIR})

target_link_libraries(soci_sqlite3 soci_core ${SQLITE3_LIBRARIES})

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR})
include_directories(avboost/include)
include_directories(avhttp/include)
include_directories(libwebqq/include)
include_directories(deCAPTCHA/include)
include_directories(avhttpd/include)

add_subdirectory(libwebqq)
add_subdirectory(libirc)
add_subdirectory(libxmpp)
add_subdirectory(libmailexchange)
add_subdirectory(deCAPTCHA)
add_subdirectory(libavbot)
add_subdirectory(libavlog)
add_subdirectory(extension)

configure_file(version.c.in version.c @ONLY)


set(AVBOT_SOURCES
	auto_welcome.hpp
	avbot_vc_feed_input.hpp
	botctl.hpp
	counter.hpp
	input.hpp
	main.cpp
	botctl.cpp
	input.cpp
	avbot_log_search.cpp
	${PROJECT_BINARY_DIR}/version.c
	${PROJECT_BINARY_DIR}/avbot.rc
)

set(AVBOTRPC_SOURCES
	rpc/server.cpp
	rpc/server.hpp
)

if(NOT WIN32)
list(APPEND AVBOT_SOURCES backtrace.cpp)
endif()

if(WIN32)
list(APPEND AVBOT_SOURCES sound.cpp )
else()

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tweet.o
	COMMAND ${CMAKE_OBJCOPY} -I binary -O ${OBJCOPYFORMAT} -B ${BINARY_ARCHITECTURE} tweet.wav ${CMAKE_CURRENT_BINARY_DIR}/tweet.o
	DEPENDS tweet.wav WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

list(APPEND AVBOT_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/tweet.o )
endif()

if(MSVC)

if(CMAKE_CL_64)
LINK_DIRECTORIES( "${CMAKE_CURRENT_SOURCE_DIR}/extension/luascript/msvc_x64/")
else()
LINK_DIRECTORIES( "${CMAKE_CURRENT_SOURCE_DIR}/extension/luascript/msvc_x86/")
endif()

endif()

add_executable(avbot ${AVBOT_SOURCES} ${AVBOTRPC_SOURCES})

if(NOT WIN32)
target_link_libraries(avbot -rdynamic)
endif()

if(WIN32)
add_subdirectory(gui)
target_link_libraries(avbot avbotgui)
if(MSVC)
target_link_libraries(avbot ssleay32.lib libeay32.lib zlib.lib crypt32.lib secur32.lib ws2_32.lib comctl32.lib mswsock.lib winmm.lib)
else()
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -lcrypt32 -lmswsock")
target_link_libraries(avbot -lssl -lcrypto -lcrypt32 -lws2_32 -lcomctl32 -lmswsock -lwinmm)
endif()
add_definitions(-U__STRICT_ANSI__)

if(WITH_ICONV AND NOT MSVC)
target_link_libraries(avbot -liconv)
endif()

else()
target_link_libraries(avbot ${OPENSSL_LIBRARIES} )
endif()

if(APPLE AND WITH_ICONV)
target_link_libraries(avbot -liconv)
endif()

target_link_libraries(avbot libavbot avlog extension)

target_link_libraries(avbot ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if(NOT MSVC)
install(TARGETS avbot RUNTIME DESTINATION bin)
endif()

if( NOT Boost_USE_STATIC_RUNTIME )
	message(STATUS "dynamic link to boost is not recomanded")
endif()

check_library_exists(rt clock_gettime "" HAVE_CLOCK_GETTIME)
if(HAVE_CLOCK_GETTIME)
        target_link_libraries(avbot -lrt)
endif(HAVE_CLOCK_GETTIME)


