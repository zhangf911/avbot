
option(ENABLE_LUA "enable lua script support" OFF)

find_package(ZLIB)
if (ZLIB_FOUND)
add_definitions(-DHAVE_ZLIB)
endif()

add_library(extension STATIC
	extension.hpp extension.cpp
	exchangerate.hpp
	html.hpp
	iplocation.hpp
	joke.hpp joke.cpp
	urlpreview.hpp	urlpreview.cpp
	bulletin.hpp bulletin.cpp
	metalprice.hpp stockprice.hpp)

include_directories(../qqwry/include)

if(ENABLE_LUA)


find_package(PkgConfig REQUIRED)

pkg_check_modules(LUAJIT REQUIRED luajit>=2.0)

add_definitions(-DENABLE_LUA)
include_directories(
	${LUAJIT_INCLUDE_DIRS}
	)

add_subdirectory(luascript)

target_link_libraries(extension libluascript)

endif()


if(ZLIB_FOUND)
target_link_libraries(extension ${ZLIB_LIBRARIES})
endif()
