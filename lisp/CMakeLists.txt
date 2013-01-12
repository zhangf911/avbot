find_package(Boost 1.48 REQUIRED COMPONENTS thread date_time filesystem system program_options regex)

find_package(Threads)

#set(CMAKE_CXX_FLAGS -std=c++11)
add_executable(lisp_test main.cpp)

target_link_libraries(lisp_test  ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})