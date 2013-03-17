find_package(Boost 1.48 COMPONENTS regex signals system locale)

add_library(pop3 pop3.cpp)
target_link_libraries(pop3 ${Boost_LIBRARIES})

if( ENABLE_TEST )

add_executable(pop3test test.cpp)
target_link_libraries(pop3test pop3 ${CMAKE_THREAD_LIBS_INIT} )

endif()