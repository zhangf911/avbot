
option(ENABLE_LUA "enable lua script support" OFF)
option(INTERNAL_LUABIND "enable lua script support" ON)

add_library(extension STATIC extension.cpp joke.cpp urlpreview.cpp bulletin.cpp )

if(ENABLE_LUA)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LUAJIT REQUIRED luajit>=2.0)

add_definitions(-DENABLE_LUA)
include_directories(
	luabind
	${LUAJIT_INCLUDE_DIRS}
	)

add_library(luascript STATIC luascript.cpp)

if(INTERNAL_LUABIND)
	add_library(luabind STATIC
			luabind/src/open.cpp
			luabind/src/create_class.cpp
			luabind/src/class.cpp
			luabind/src/class_info.cpp
			luabind/src/class_rep.cpp
			luabind/src/class_registry.cpp
			luabind/src/object_rep.cpp
			luabind/src/scope.cpp
			luabind/src/function.cpp
			luabind/src/exception_handler.cpp
			luabind/src/error.cpp
			luabind/src/inheritance.cpp
			luabind/src/pcall.cpp
			luabind/src/link_compatibility.cpp
		)
	target_link_libraries(luabind ${LUAJIT_LIBRARIES})
	target_link_libraries(luascript luabind)
else()
	target_link_libraries(luascript -lluabind ${LUAJIT_LIBRARIES})
endif()

target_link_libraries(extension luascript)

add_subdirectory(slnunicode)

add_custom_command(TARGET avbot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/libs"
        $<TARGET_FILE_DIR:avbot>)

endif()

