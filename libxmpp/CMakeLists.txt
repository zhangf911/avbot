find_package(Boost 1.55 REQUIRED COMPONENTS date_time regex system coroutine context atomic )
include_directories(${Boost_INCLUDE_DIRS})

option(INTERNALGLOOX "use bundled gloox" ON)

if(INTERNALGLOOX)
add_subdirectory(gloox)
include_directories(gloox)
endif()

add_library(xmpp STATIC xmpp_impl.hpp xmpp_impl.cpp xmpp.hpp xmpp.cpp)
target_link_libraries(xmpp ${Boost_LIBRARIES})

if(INTERNALGLOOX)
target_link_libraries(xmpp gloox)
else()
target_link_libraries(xmpp -lgloox)
endif()

if(ENABLE_TEST)
	add_executable(xmpptest test.cpp)
	target_link_libraries(xmpptest xmpp)
endif()
