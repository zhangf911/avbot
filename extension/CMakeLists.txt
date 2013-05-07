
option(ENABLE_LUA "enable lua script support" OFF)

add_library(extension STATIC extension.cpp joke.cpp urlpreview.cpp bulletin.cpp )

if(ENABLE_LUA)


find_package(PkgConfig REQUIRED)

pkg_check_modules(LUAJIT REQUIRED luajit>=2.0)

add_definitions(-DENABLE_LUA)
include_directories(
	${LUAJIT_INCLUDE_DIRS}
	)

add_subdirectory(luascript)

target_link_libraries(extension luascript)

endif()

