
EXTRA_DIST = CMakeLists.txt README.md

AM_CPPFLAGS = -I${top_srcdir} -I${top_srcdir}/avhttp/include -I${top_srcdir}/avproxy/include

noinst_LTLIBRARIES = libextension.la libjoke.la

libextension_la_SOURCES = joke.hpp joke.cpp urlpreview.hpp urlpreview.cpp extension.hpp extension.cpp

libextension_la_LDFLAGS = -static

libextension_la_LIBADD =  ${BOOST_SYSTEM_LIBS} ${BOOST_FILESYSTEM_LIBS}
