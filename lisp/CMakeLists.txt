set(CMAKE_CXX_FLAGS -std=c++11)
add_executable(lisp_test main.cpp)
target_link_libraries(lisp_test "-lboost_system -lboost_thread -lpthread")
