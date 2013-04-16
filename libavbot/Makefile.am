
EXTRA_DIST = CMakeLists.txt

AM_CPPFLAGS = -I${top_srcdir}/avproxy/include -I${top_srcdir}

noinst_LTLIBRARIES = libavbot.la

libmx_la_SOURCES = avbot.cpp avbot.hpp

libmx_la_LDFLAGS = -static

libmx_la_LIBADD = ${BOOST_SYSTEM_LIBS} ${BOOST_REGEX_LIBS} ${BOOST_LOCALE_LIBS}
