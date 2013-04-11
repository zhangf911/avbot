
EXTRA_DIST = m4 avproxy boost libirc CMakeLists.txt fsconfig.ipp  input.ipp avbot.rc

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AUTOMAKE_OPTIONS = color-tests parallel-tests

SUBDIRS = libmailexchange libwebqq libxmpp libirc

AM_CPPFLAGS = -I ${srcdir}/avproxy/include

bin_PROGRAMS = avbot

avbot_SOURCES = main.cpp botctl.cpp messagegroup.cpp \
		auto_question.hpp  botctl.hpp  counter.hpp  logger.hpp  messagegroup.hpp resource.h

avbot_LDFLAGS=${BOOST_THREAD_LDFLAGS}

avbot_LDADD = ${top_builddir}/libxmpp/libxmpp.la ${top_builddir}/libwebqq/libwebqq.la ${top_builddir}/libmailexchange/libmx.la

avbot_LDADD +=${BOOST_THREAD_LIBS} ${BOOST_SYSTEM_LIBS} ${BOOST_PROGRAM_OPTIONS_LIBS} ${BOOST_FILESYSTEM_LIBS}  ${BOOST_REGEX_LIBS} ${BOOST_SIGNALS_LIBS} ${BOOST_LOCALE_LIBS}