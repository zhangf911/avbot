#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([avbot], [3.6], [discuss@avplayer.org])
AC_CONFIG_SRCDIR([main.cpp])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([locale.h stdint.h stdlib.h string.h unistd.h wchar.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

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
                 libxmpp/Makefile
                 libxmpp/gloox/Makefile])

AC_CONFIG_SUBDIRS([libxmpp/gloox])
AC_OUTPUT
