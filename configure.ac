#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

# BOOST_LOCALE([PREFERRED-RT-OPT])
# --------------------------------
# Look for Boost.System.  For the documentation of PREFERRED-RT-OPT, see the
# documentation of BOOST_FIND_LIB above.  This library was introduced in Boost
# 1.35.0.
BOOST_DEFUN([Locale],
[BOOST_FIND_LIB([locale], [$1],
                [boost/locale.hpp],
                [boost::locale::conv::between( "nihao", "UTF-8", "GBK" );])
])# BOOST_LOCALE

# BOOST_THREAD([PREFERRED-RT-OPT])
# --------------------------------
# Look for Boost.System.  For the documentation of PREFERRED-RT-OPT, see the
# documentation of BOOST_FIND_LIB above.  This library was introduced in Boost
# 1.35.0.
BOOST_DEFUN([THREAD],
[BOOST_FIND_LIB([thread], [$1],
                [boost/thread.hpp],
                [;])
])# BOOST_THREAD


AC_INIT([avbot], [3.6], [discuss@avplayer.org])
AC_CONFIG_SRCDIR([main.cpp])
AC_CONFIG_MACRO_DIR([m4])

LT_INIT

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

AC_LANG(C++)

# Checks for libraries.

  dnl OpenSSL
  AC_ARG_WITH(openssl, AC_HELP_STRING([--with-openssl=[DIR]],
        [Support Stream Encryption (needs OpenSSL)]),
        openssl=$withval, openssl=yes)
  if test "$openssl" != "no"; then
    if test "$openssl" != "yes"; then
      LDFLAGS="${LDFLAGS} -L$openssl/lib"
      CPPFLAGS="${CPPFLAGS} -I$openssl/include"
    fi
    LIBS_TMP=${LIBS}
    LIBS="${LIBS} -lssl -lcrypto"
    AC_CHECK_HEADER(openssl/ssl.h,
    AC_CHECK_LIB(ssl, SSL_library_init,
            [openssl=yes], openssl=no),
            [openssl=no LIBS=${LIBS_TMP}])
  fi
  if test "$openssl" != "no" ; then
    AC_DEFINE(HAVE_OPENSSL, 1, [Define to 1 if you want TLS support (OpenSSL). Undefine HAVE_GNUTLS.])
  else
    AC_MSG_WARN([OpenSSL not found])
  fi
  AC_MSG_CHECKING([if OpenSSL should be used])
  AC_MSG_RESULT($openssl)



BOOST_REQUIRE(1.48)
BOOST_LOCALE([s])
BOOST_THREAD([s])
BOOST_SYSTEM([s])
BOOST_FILESYSTEM([s])
BOOST_DATE_TIME([s])
BOOST_PROGRAM_OPTIONS([s])
BOOST_REGEX([s])
BOOST_SIGNALS([s])

BOOST_FORMAT
BOOST_BIND
BOOST_FUNCTION
BOOST_ASIO

AC_DEFINE_UNQUOTED(BOOST_THREAD_USE_LIB,"1",[define this to use static version of boost thread])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([dup2 gettimeofday memset putenv setlocale socket strchr strcspn strstr strtol])

AC_CONFIG_FILES([
                 Makefile
                 libirc/Makefile
                 libmailexchange/Makefile
                 libwebqq/Makefile
                 libxmpp/Makefile])

AC_CONFIG_SUBDIRS([libxmpp/gloox])

AC_OUTPUT
