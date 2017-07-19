#!/bin/bash

# Process this file with autoconf to produce a configure script.
AC_INIT(opensand-rt, 4.2.0, [http://opensand.org])
AC_CONFIG_SRCDIR([src])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE
AC_LANG_PUSH([C++])

# Disable static libraries by default
AC_DISABLE_STATIC

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdio.h stdlib.h stdint.h string.h strings.h assert.h  endian.h \
                  pthread.h arpa/inet.h fcntl.h malloc.h netdb.h netinet/in.h \
                  stddef.h sys/ioctl.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([gethostbyname gettimeofday inet_ntoa memset mkdir select socket sqrt strchr strerror strstr])

# check for pkg-config
PKG_PROG_PKG_CONFIG

# check for opensand-output library
PKG_CHECK_MODULES([OPENSAND_OUTPUT], [opensand_output = 4.2.0])
LIBS="$OPENSAND_OUTPUT_LIBS $LIBS"
AC_SUBST(AM_CPPFLAGS, "$OPENSAND_OUTPUT_CFLAGS $AM_CPPFLAGS")


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

AC_DEFINE(ENABLE_TCMALLOC, [0], [use fast and multi-threaded malloc])
TCMALLOC=""
GPERFTOOLS_INC=""
AC_ARG_ENABLE(tcmalloc,
              AS_HELP_STRING([--enable-tcmalloc],
                             [use fast and multi-threaded malloc [[default=yes]]]),
              tcmalloc=$enableval,
              tcmalloc=yes)
if test "x$tcmalloc" != "xno"; then
	TCMALLOC="-ltcmalloc"
	AC_DEFINE(ENABLE_TCMALLOC, [1], [use fast and multi-threaded malloc])
	# debian installs gperftools header in google subfolder while this should be in gperftools...
	AC_CHECK_HEADER([gperftools/heap-checker.h], [GPERFTOOLS_INC="/usr/include/gperftools"], [\
	AC_CHECK_HEADER([google/heap-checker.h], [GPERFTOOLS_INC="/usr/include/google"], [AC_MSG_ERROR("Could not find google perftools headers")])])
fi

AC_SUBST(AM_CPPFLAGS, "$AM_CPPFLAGS -g -Wall ${WERROR} -DUTI_DEBUG_ON -I${GPERFTOOLS_INC}")
AC_SUBST(AM_LDFLAGS, "$AM_LDFLAGS ${TCMALLOC} -lpthread -lrt")

AM_DEP_TRACK

AC_CONFIG_FILES([Makefile \
                 src/Makefile \
                 src/test/Makefile \
                 opensand_rt.pc \
                 ])

AC_OUTPUT

