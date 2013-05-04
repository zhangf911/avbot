
option(ENABLE_LUA "enable lua script support" OFF)
if(ENABLE_LUA)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LUAJIT REQUIRED luajit>=2.0)

endif()
