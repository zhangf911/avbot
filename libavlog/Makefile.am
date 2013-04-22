
EXTRA_DIST = CMakeLists.txt

AM_CPPFLAGS = -I${top_srcdir}

noinst_LTLIBRARIES = libavlog.la

libavlog_la_SOURCES = avlog.cpp avlog.hpp

libavlog_la_LDFLAGS = -static

libavlog_la_LIBADD = ${BOOST_SYSTEM_LIBS} ${BOOST_FILESYSTEM_LIBS}
