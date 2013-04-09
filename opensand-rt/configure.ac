#!/bin/bash

# Process this file with autoconf to produce a configure script.
AC_INIT(opensand-rt, trunk, [http://opensand.org])
AC_CONFIG_SRCDIR([src/Block.cpp])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE(opensand-rt, trunk)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_LIBTOOL

# Checks for libraries.
AC_SUBST(SET_MAKE,MAKE=make)

# Checks for header files.
AC_HEADER_STDC
#AC_CHECK_HEADERS([errno.h fcntl.h stdlib.h string.h sys/time.h unistd.h])
AC_CHECK_HEADERS([arpa/inet.h fcntl.h malloc.h netdb.h netinet/in.h stddef.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME


# check for pkg-config
PKG_PROG_PKG_CONFIG

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([gethostbyname gettimeofday inet_ntoa memset mkdir select socket sqrt strchr strerror strstr])

AC_DEFINE_UNQUOTED([MEMORY_POOL], [0], [Whether memory pool is enabled or not.])

# check if -Werror must be appended to CFLAGS
WERROR=""
AC_ARG_ENABLE(fail_on_warning,
              AS_HELP_STRING([--enable-fail-on-warning],
                             [build fails on warnings if enabled [[default=no]]]),
              fail_on_warning=$enableval,
              fail_on_warning=no)
if test "x$fail_on_warning" != "xno"; then
	WERROR="-Werror"
fi

AC_SUBST(AM_CPPFLAGS, "$AM_CPPFLAGS -g ${WERROR} -DUTI_DEBUG_ON")
# TODO Wall
#AC_SUBST(AM_CPPFLAGS, "$AM_CPPFLAGS -g -Wall ${WERROR} -DUTI_DEBUG_ON")


AM_DEP_TRACK

AC_CONFIG_FILES([ \
	Makefile \
	src/Makefile \
	test_runtime/Makefile \
	doc_src/Makefile \
	opensand_rt.pc \
	])
AC_OUTPUT

