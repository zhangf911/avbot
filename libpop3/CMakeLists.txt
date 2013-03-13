

add_executable(test pop3.cpp)

target_link_libraries(test ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )