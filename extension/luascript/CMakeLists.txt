
project(avbotluascript)

if(ENABLE_LUA)

add_definitions(-DENABLE_LUA)
include_directories(
	luabind
	${LUAJIT_INCLUDE_DIRS}
	)

add_library(libluascript STATIC luascript.cpp
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
target_link_libraries(libluascript ${LUAJIT_LIBRARIES})

add_custom_command(TARGET avbot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory libs $<TARGET_FILE_DIR:avbot>)

endif()
