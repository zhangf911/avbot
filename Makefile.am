
EXTRA_DIST = m4 avproxy boost libirc CMakeLists.txt fsconfig.ipp  input.ipp avbot.rc avbot.manifest

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AUTOMAKE_OPTIONS = color-tests parallel-tests

SUBDIRS = libmailexchange libwebqq libxmpp libirc libavlog libavbot

AM_CPPFLAGS = -I ${srcdir}/avproxy/include

bin_PROGRAMS = avbot

avbot_SOURCES = main.cpp botctl.cpp avbot_rpc_server.cpp avbot_rpc_server.hpp \
		auto_question.hpp  botctl.hpp  counter.hpp resource.h
if OS_WINNT
avbot_SOURCES += avbot.rc
endif

avbot_LDFLAGS=${BOOST_THREAD_LDFLAGS}

avbot_LDADD = ${top_builddir}/libavbot/libavbot.la ${top_builddir}/libavlog/libavlog.la

avbot_LDADD +=${BOOST_THREAD_LIBS} ${BOOST_SYSTEM_LIBS} ${BOOST_PROGRAM_OPTIONS_LIBS} ${BOOST_FILESYSTEM_LIBS}  ${BOOST_REGEX_LIBS} ${BOOST_SIGNALS_LIBS} ${BOOST_LOCALE_LIBS}

.rc.o:
	${RC} -o $@ $<
