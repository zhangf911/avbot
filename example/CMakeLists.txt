
find_package(Boost 1.54 REQUIRED system regex)

add_executable(avhttpd main.cpp)

target_link_libraries(avhttpd ${Boost_LIBRARIES})

install(TARGETS avhttpd RUNTIME DESTINATION bin)

include_directories(../include)