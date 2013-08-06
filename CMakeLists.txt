project(avhttpd)

add_executable(avhttpd main.cpp)

install(TARGETS avhttpd RUNTIME DESTINATION bin)
