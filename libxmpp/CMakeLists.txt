find_package(Boost 1.48 REQUIRED COMPONENTS date_time regex signals system)
include_directories(${Boost_INCLUDE_DIRS})

option(INTERNALGLOOX "use bundled gloox" ON)

if(INTERNALGLOOX)
add_subdirectory(gloox)
include_directories(gloox)
endif()

add_library(libxmpp STATIC xmpp_impl.cpp xmpp.cpp)
target_link_libraries(libxmpp ${Boost_LIBRARIES})

if(INTERNALGLOOX)
target_link_libraries(libxmpp gloox)
else()
target_link_libraries(libxmpp -lgloox)
endif()

if(ENABLE_TEST)
	add_executable(test test.cpp)
	target_link_libraries(test libasio libxmpp)
endif()
