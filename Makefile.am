
EXTRA_DIST = m4 avproxy avhttp boost libirc CMakeLists.txt fsconfig.ipp  input.ipp avbot.rc avbot.manifest

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AUTOMAKE_OPTIONS = color-tests parallel-tests

SUBDIRS = libmailexchange libwebqq libxmpp libirc libavbot libavlog libjoke

AM_CPPFLAGS = -I ${srcdir}/avproxy/include -I ${srcdir}/avhttp/include

bin_PROGRAMS = avbot

avbot_SOURCES = main.cpp botctl.cpp avbot_rpc_server.cpp avbot_rpc_server.hpp \
		auto_welcome.hpp  botctl.hpp  counter.hpp resource.h
if OS_WINNT
avbot_SOURCES += avbot.rc
endif

avbot_LDFLAGS=${BOOST_THREAD_LDFLAGS}

avbot_LDADD = ${top_builddir}/libavbot/libavbot.la ${top_builddir}/libavlog/libavlog.la ${top_builddir}/libjoke/libjoke.la

avbot_LDADD +=${BOOST_THREAD_LIBS} ${BOOST_SYSTEM_LIBS} ${BOOST_PROGRAM_OPTIONS_LIBS} ${BOOST_FILESYSTEM_LIBS}  ${BOOST_REGEX_LIBS} ${BOOST_SIGNALS_LIBS} ${BOOST_LOCALE_LIBS}

.rc.o:
	${RC} -o $@ $<

rpm: dist-bzip2 @PACKAGE_NAME@.spec
	rpmbuild -bb \
		--define "_sourcedir `pwd`" \
		--define "_builddir `pwd`" \
		--define "_specdir `pwd`" \
		--define "_rpmdir `pwd`" \
		--define "_srcrpmdir `pwd`" \
		@PACKAGE_NAME@.spec

rpm32: dist-bzip2 @PACKAGE_NAME@.spec
	CXXFLAGS="-m32 -Os -g0" CFLAGS="-m32 -Os -g0" rpmbuild -bb --target=i686 \
		--define "_sourcedir `pwd`" \
		--define "_builddir `pwd`" \
		--define "_specdir `pwd`" \
		--define "_rpmdir `pwd`" \
		--define "_srcrpmdir `pwd`" \
		@PACKAGE_NAME@.spec