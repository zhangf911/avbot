
EXTRA_DIST = CMakeLists.txt README.md

AM_CPPFLAGS = -I${top_srcdir} -I${top_srcdir}/avhttp/include -I${top_srcdir}/avproxy/include @LUAJIT_CFLAGS@

noinst_LTLIBRARIES = libextension.la

libextension_la_SOURCES = joke.hpp joke.cpp urlpreview.hpp urlpreview.cpp extension.hpp extension.cpp bulletin.hpp bulletin.cpp metalprice.hpp httpagent.hpp

libextension_la_LDFLAGS = -static

libextension_la_LIBADD =  ${BOOST_SYSTEM_LIBS} ${BOOST_FILESYSTEM_LIBS}

if ENABLE_LUA

SUBDIRS = luascript

libextension_la_LIBADD += ${builddir}/luascript/libluascript.la

endif
