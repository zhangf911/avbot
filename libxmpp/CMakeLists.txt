find_package(Boost 1.48 REQUIRED COMPONENTS date_time regex signals system)

find_library(GLOOX_LIBRARIES NAMES gloox)

if(EXISTS ${GLOOX_LIBRARIES})
	message( STATUS ${GLOOX_LIBRARIES} " found")
else()
	message( FATAL_ERROR  "gloox not found")
endif()

include_directories(${Boost_INCLUDE_DIRS})

add_library(libxmpp STATIC xmpp_impl.cpp xmpp.cpp)

target_link_libraries(libxmpp ${Boost_LIBRARIES} ${GLOOX_LIBRARIES} -pthread)

add_executable(test test.cpp)

target_link_libraries(test libxmpp)
