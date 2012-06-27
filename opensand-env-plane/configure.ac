#!/bin/bash

# Process this file with autoconf to produce a configure script.
AC_INIT(opensand-env-plane, 0.9.0, [Viveris Technologies])
AC_CONFIG_SRCDIR([src])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Disable static libraries by default
AC_DISABLE_STATIC

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdio.h stdlib.h stdint.h string.h strings.h assert.h arpa/inet.h endian.h pthread.h])

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
AC_CHECK_FUNCS([malloc free])

# check for pkg-config
PKG_PROG_PKG_CONFIG

AC_SUBST(AM_CFLAGS, "-g -Wall -W -Werror")
#AC_SUBST(AM_CFLAGS, "-g -Wall -W -Werror -Wstrict-prototypes")

AM_DEP_TRACK

AC_CONFIG_FILES([Makefile \
                 src/Makefile \
                 opensand_env_plane.pc \
                 ])

AC_OUTPUT

