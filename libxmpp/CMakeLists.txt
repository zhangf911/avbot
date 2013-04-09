find_package(Boost 1.48 REQUIRED COMPONENTS date_time regex signals system)
find_package(OpenSSL REQUIRED)

if(NOT WIN32)

	find_package(ZLIB )
	if(ZLIB_FOUND)
		add_definitions(-DHAVE_ZLIB)
	endif()

endif()


include_directories(${OpenSSL_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

option(INTERNALGLOOX "use bundled gloox" ON)

if(INTERNALGLOOX)
add_subdirectory(gloox)
include_directories(gloox)
add_library(xmpp STATIC $<TARGET_OBJECTS:gloox> xmpp_impl.cpp xmpp.cpp)
else()
add_library(xmpp STATIC xmpp_impl.cpp xmpp.cpp)
endif()

target_link_libraries(xmpp ${Boost_LIBRARIES} ${ZLIB_LIBRARIES} ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if(INTERNALGLOOX)
target_link_libraries(xmpp )
else()
target_link_libraries(xmpp -lgloox)
endif()

if(ENABLE_TEST)
	add_executable(xmpptest test.cpp)
	target_link_libraries(xmpptest xmpp)
endif()
