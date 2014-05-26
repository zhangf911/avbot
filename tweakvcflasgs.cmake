if(MSVC)

set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_MinSizeRel
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_MinSizeRel
		CMAKE_EXE_LINKER_FLAGS
		CMAKE_EXE_LINKER_FLAGS_DEBUG
		CMAKE_EXE_LINKER_FLAGS_RELEASE
		CMAKE_EXE_LINKER_FLAGS_MinSizeRel
   )

foreach(CompilerFlag ${CompilerFlags})
  string(REPLACE "/MDd" "/MTd" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

foreach(CompilerFlag ${CompilerFlags})
  string(REPLACE "/W3" "/W1" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

foreach(CompilerFlag ${CompilerFlags})
  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /MP")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DEBUG")

#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS}  /ignore:4099 /DEBUG /MTd /NODEFAULTLIB:libcmt.lib")
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER}  /SAFESEH:NO")
