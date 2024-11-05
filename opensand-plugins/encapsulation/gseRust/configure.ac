#!/bin/bash

# Process this file with autoconf to produce a configure script.
m4_define([opensand_version], m4_bregexp(m4_quote(m4_esyscmd_s([cat ChangeLog])), [[0-9]+\.[0-9]+\.[0-9]+], [\&]))
AC_INIT(opensand-gseRust-encap-plugin, [opensand_version], [http://opensand.org])
AC_CONFIG_SRCDIR([src])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Disable static libraries by default
AC_DISABLE_STATIC

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdio.h stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_CHECK_FUNCS([])

# check for pkg-config
PKG_PROG_PKG_CONFIG
CXXFLAGS="$CXXFLAGS -std=c++17"

# check for Rust compiler
AC_CHECK_PROG(CARGO, [cargo], [yes], [no])
AC_CHECK_PROG(RUSTC, [rustc], [yes], [no])
AS_IF([test "$CARGO" != "yes" -o "$RUSTC" != "yes"], [
    AC_MSG_FAILURE("Missing Rust toolchain: cargo $CARGO / rustc $RUSTC")
])

# check for opensand-output library
PKG_CHECK_MODULES([OPENSAND_OUTPUT], [opensand_output = opensand_version])
LIBS="$OPENSAND_OUTPUT_LIBS $LIBS"
AC_SUBST(AM_CPPFLAGS, "$OPENSAND_OUTPUT_CFLAGS $AM_CPPFLAGS")

# check for opensand-plugin library
PKG_CHECK_MODULES([OPENSAND_PLUG], [opensand_plugin = opensand_version])
LIBS="$OPENSAND_PLUG_LIBS $LIBS"
AC_SUBST(AM_CPPFLAGS, "$OPENSAND_PLUG_CFLAGS $AM_CPPFLAGS")

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

#### not treating unused var / funct as error 
AC_SUBST(AM_CPPFLAGS, "-g -Wall -Wno-error=unused-variable -Wno-error=unused-function ${WERROR} -DUTI_DEBUG_ON $AM_CPPFLAGS")

AM_DEP_TRACK

AC_CONFIG_FILES([\
    Makefile \
    src/Makefile \
])

AC_OUTPUT

