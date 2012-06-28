#!/bin/bash

# Process this file with autoconf to produce a configure script.
AC_INIT(opensand-margouilla, 0.9.0, [Viveris Technologies])
AC_CONFIG_SRCDIR([src/mgl_bloc.cpp])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE(opensand-margouilla, 0.9.0)

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

# check if memory pool is enabled
AC_ARG_ENABLE(memory_pool,
              AS_HELP_STRING([--enable-memory-pool],
                             [enable MEMORY POOL [[default=yes]]]),
              enable_memory_pool=$enableval,
              enable_memory_pool=yes)
if test "$enable_memory_pool" = "yes"; then
   memory_pool=1
else
   memory_pool=0
fi
AC_DEFINE_UNQUOTED([MEMORY_POOL], [$memory_pool], [Whether memory pool is enabled or not.])

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
	opensand_margouilla.pc \
	])
AC_OUTPUT

