
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.50)
AC_INIT([gloox], [1.0], [js@camaya.net])
AC_CONFIG_SRCDIR(gloox/gloox.cpp)
AM_INIT_AUTOMAKE([foreign])

dnl create a config.h file (Automake will add -DHAVE_CONFIG_H)
AC_CONFIG_HEADER(config.h.unix)

AC_SUBST(VERSION)

ISODATE=`date +%Y-%m-%d`
AC_SUBST(ISODATE)

AC_CANONICAL_HOST

dnl Checks for programs.
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_CXX

LT_INIT

dnl
dnl extra paths
dnl

AC_ARG_WITH(extra_include_path, AC_HELP_STRING([--with-extra-include-path],
                                           [use additional include paths]),
            extra_include_path=$withval)
split_includes="`echo $extra_include_path | sed -e 's/:/ /g'`"
for incpath in $split_includes ; do
    CPPFLAGS="-I$incpath $CPPFLAGS"
done

AC_ARG_WITH(extra_library_path, AC_HELP_STRING([--with-extra-library-path],
                                               [use additional library paths]),
            extra_library_path=$withval)
split_libs="`echo $extra_library_path | sed -e 's/:/ /g'`"
for libpath in $split_libs ; do
    LDFLAGS="-L$libpath $LDFLAGS"
done


dnl extra win32 libs
case "$host_os" in
  *mingw*|*cygwin*)
      LIBS="${LIBS} -lwsock32 -lgdi32 -ldnsapi -lcrypt32 -ldnsapi -lsecur32";;
  *)
      CXXFLAGS="$CXXFLAGS -ansi";;
esac

ssl=no

dnl schannel
AC_ARG_WITH(schannel, AC_HELP_STRING([--with-schannel],
      [Support Stream Encryption (win32 only)]),
      schannel=yes, schannel=no)
if test "$schannel" = "yes"; then
  case "$host_os" in
    *mingw*|*cygwin*)
        ssl=yes
        LIBS="${LIBS} -lcrypt32 -lsecur32";;
    *)
        schannel=no
        echo "*"
        echo "* You provided the '--with-schannel' switch. SChannel is only available in MinGW/cygwin. Ignoring."
        echo "*";;
  esac
  AC_MSG_CHECKING([if SChannel should be used])
  AC_MSG_RESULT($schannel)
fi

dnl OpenSSL
if test "$ssl" = "no"; then
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
fi

dnl Zlib
AC_ARG_WITH(zlib, AC_HELP_STRING([--with-zlib=[DIR]],
      [Support Stream Compression (needs Zlib)]),
      zlib=$withval, zlib=yes)
if test "$zlib" != "no"; then
  if test "$zlib" != "yes"; then
    LDFLAGS="${LDFLAGS} -L$zlib/lib"
    CPPFLAGS="${CPPFLAGS} -I$zlib/include"
  fi
  AC_CHECK_HEADER(zlib.h,
  AC_CHECK_LIB(z, zlibVersion,
           [zlib=yes LIBS="${LIBS} -lz"], zlib=no),
           zlib=no)
fi
if test "$zlib" != "no" ; then
  AC_DEFINE(HAVE_ZLIB, 1, [Define to 1 if you want Stream Compression support.])
else
  AC_MSG_WARN([Zlib not found])
fi
AC_MSG_CHECKING([if Zlib should be used])
AC_MSG_RESULT($zlib)

getaddrinfo="no"
AC_ARG_ENABLE( getaddrinfo,
               [  --enable-getaddrinfo    use getaddrinfo for dns lookups [default=no]],
               [getaddrinfo="yes"] )
if test "x$getaddrinfo" = "xyes"; then
    AC_CHECK_FUNCS(getaddrinfo,,getaddrinfo="no")
fi
AC_MSG_CHECKING([whether to use getaddrinfo])
AC_MSG_RESULT($getaddrinfo)

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h strings.h errno.h arpa/nameser.h)
AC_CHECK_FUNCS(setsockopt,,[AC_CHECK_LIB(socket,setsockopt)])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE


dnl Generating makefiles.
AC_CONFIG_FILES([
Makefile
gloox/Makefile
])
AC_OUTPUT
