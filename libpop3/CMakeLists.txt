option(BOOST_USE_ICU "if your boost has icu backend, set it to ON" OFF)

find_package(Boost 1.48 COMPONENTS regex signals system locale)

add_library(pop3 pop3.cpp)
target_link_libraries(pop3 ${Boost_LIBRARIES})
if(BOOST_USE_ICU)
	target_link_libraries(pop3 -licuuc)
endif()

if( ENABLE_TEST )
add_definitions(-DDEBUG)
add_executable(pop3test test.cpp)
target_link_libraries(pop3test pop3 ${CMAKE_THREAD_LIBS_INIT} )

endif()