
EXTRA_DIST = CMakeLists.txt test.cpp

SUBDIRS = gloox

AM_CPPFLAGS = -I ${srcdir}/gloox -I ${top_srcdir} -I ${top_srcdir}/avproxy/include

noinst_LTLIBRARIES = libxmpp.la

libxmpp_la_SOURCES = xmpp.h xmpp.cpp xmpp_impl.h xmpp_impl.cpp

libxmpp_la_LDFLAGS = -static

libxmpp_la_LIBADD = ${builddir}/gloox/gloox/libgloox.la