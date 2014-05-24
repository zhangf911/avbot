file(GLOB SOCI_CORE_SOURCES RELATIVE ${PROJECT_SOURCE_DIR} soci/src/core/*.cpp)

add_library(soci_core STATIC ${SOCI_CORE_SOURCES})

file(GLOB SOCI_SQLITE3_SOURCES RELATIVE ${PROJECT_SOURCE_DIR} soci/src/backends/sqlite3/*.cpp)
add_library(soci_sqlite3 STATIC ${SOCI_SQLITE3_SOURCES})

configure_file(soci/src/core/soci_backends_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/soci_backends_config.h)

if(NOT WIN32)
  set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/soci/src/cmake ${CMAKE_MODULE_PATH})
  set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/soci/src/cmake/modules ${CMAKE_MODULE_PATH})
  set(DL_FIND_QUIETLY TRUE)
  find_package(DL)
  if(DL_FOUND)
    include_directories(${DL_INCLUDE_DIR})
    target_link_libraries(soci_core ${DL_LIBRARY})
  endif()
endif()

add_definitions(-DSOCI_LIB_PREFIX="" -DSOCI_LIB_SUFFIX="")
