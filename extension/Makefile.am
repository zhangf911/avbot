
EXTRA_DIST = CMakeLists.txt README.md

AM_CPPFLAGS = -I${top_srcdir} -I${top_srcdir}/avhttp/include

noinst_LTLIBRARIES = libextension.la libjoke.la

libextension_la_SOURCES = urlpreview.hpp extension.cpp

libjoke_la_SOURCES = joke.hpp joke.cpp

libjoke_la_LDFLAGS = -static

libjoke_la_LIBADD = ${BOOST_SYSTEM_LIBS} ${BOOST_FILESYSTEM_LIBS}

libextension_la_LDFLAGS = -static

libextension_la_LIBADD = libjoke.la
