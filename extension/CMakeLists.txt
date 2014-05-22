
option(ENABLE_LUA "enable lua script support" OFF)
option(ENABLE_PYTHON "enable python script support" OFF)

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
	metalprice.hpp stockprice.hpp
	staticcontent.hpp
	staticcontent.cpp
	pythonscriptengine.hpp
	pythonscriptengine.cpp
	dllextension.hpp)

include_directories(../qqwry/include)

if(ENABLE_LUA)

# 检查 windows 环境， 是的话，启用 delay load 支持

if(NOT WIN32)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LUAJIT REQUIRED luajit>=2.0)

add_definitions(-DENABLE_LUA)
include_directories(
	${LUAJIT_INCLUDE_DIRS}
	)

else(NOT WIN32)

# windows 环境当然不是使用 pkgconfig 啦，咱使用的是 avbot 源码里带的，嘻嘻

add_definitions(-DENABLE_LUA)
include_directories(luascript/luainclude/)
include_directories(luascript/luainclude/luajit-2.0)

if(MSVC)

if(CMAKE_CL_64)
set(LUAJIT_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/luascript/msvc_x64/lua51.lib")
else()
set(LUAJIT_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/luascript/msvc_x86/lua51.lib")
endif()

if(MSVC)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:lua51.dll" )
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:lua51.dll" PARENT_SCOPE)

endif()

else(MSVC)
# if mingw
set(LUAJIT_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/../libsformingw64/liblua51.dll.a)
endif(MSVC)

endif(NOT WIN32)

add_subdirectory(luascript)

target_link_libraries(extension libluascript)

endif(ENABLE_LUA)


if(ENABLE_PYTHON)
	add_definitions(-DENABLE_PYTHON)
	find_package(PythonLibsNew REQUIRED)
	include_directories(${PYTHON_INCLUDE_DIRS})
	link_directories(${PYTHON_LIBRARIE_DIRS})

	find_package(Boost REQUIRED COMPONENTS python)
	target_link_libraries(extension ${PYTHON_LIBRARIES} ${Boost_PYTHON_LIBRARY})
	message("extension ${PYTHON_LIBRARIES} ${Boost_PYTHON_LIBRARY}")
endif(ENABLE_PYTHON)


if(ZLIB_FOUND)
target_link_libraries(extension ${ZLIB_LIBRARIES})
endif()
