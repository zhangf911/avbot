
EXTRA_DIST = CMakeLists.txt

AM_CPPFLAGS = -I${top_srcdir} -I${top_srcdir}/avhttp/include

noinst_LTLIBRARIES = libjoke.la

libjoke_la_SOURCES = joke.hpp joke.cpp

libjoke_la_LDFLAGS = -static

libjoke_la_LIBADD = ${BOOST_SYSTEM_LIBS} ${BOOST_FILESYSTEM_LIBS}
