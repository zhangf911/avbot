
AM_CPPFLAGS = -I${top_srcdir}/avproxy/include -I${top_srcdir}

lib_LTLIBRARIES = libmx.la

libmx_la_SOURCES = pop3.cpp smtp.cpp 

libmx_la_LDFLAGS = -static

libmx_la_LIBADD = ${BOOST_SYSTEM_LIBS} ${BOOST_REGEX_LIBS} ${BOOST_SIGNALS_LIBS} ${BOOST_LOCALE_LIBS}