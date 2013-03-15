
add_library(pop3 pop3.cpp)
add_executable(test test.cpp)
target_link_libraries(test pop3)

target_link_libraries(test ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )