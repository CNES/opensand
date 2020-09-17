#!/bin/bash

# Process this file with autoconf to produce a configure script.
AC_INIT(opensand-conf, 5.2.0, [http://opensand.org])
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
AC_CHECK_HEADER([stdio.h], [], [AC_MSG_ERROR([Missing header stdio.h])])
AC_CHECK_HEADER([stdlib.h], [], [AC_MSG_ERROR([Missing header stdlib.h])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_CHECK_FUNCS([])

AC_SUBST([OPENSAND_CONF_REQ], ['libxml++-2.6 >= 2.10.0'])
PKG_CHECK_MODULES([LIBXMLXX], [$OPENSAND_CONF_REQ])
LIBS="$LIBXMLXX_LIBS $LIBS"
AC_SUBST(AM_CPPFLAGS, "$LIBXMLXX_CFLAGS")

# Check Catch2 dependency for tests
AC_LANG([C++])
AC_CHECK_HEADER([catch.hpp], [], [AC_MSG_ERROR([Missing header catch.hpp])])
AC_LANG([C])

# Check python3-dev dependency for Python module
AC_SUBST([PYTHON_REQ], ['python3'])
PKG_CHECK_MODULES([LIBPYTHON], [$PYTHON_REQ])
AC_SUBST(PY_LDADD, "$LIBPYTHON_LIBS")
AC_SUBST(PY_CPPFLAGS, "$LIBPYTHON_CFLAGS")

# Check boost-python dependency for Python module
#AC_LANG([C++])
#AC_CHECK_HEADERS([Python.h boost/python.hpp], [BOOST_PYTHON=yes; break;])
#AS_IF([test "_$BOOST_PYTHON" != "_yes"], [AC_MSG_ERROR([Missing Boost Python])])
#AC_LANG([C])

# check for pkg-config
PKG_PROG_PKG_CONFIG
CXXFLAGS="$CXXFLAGS -std=c++17"

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

AC_SUBST(AM_CPPFLAGS, "-g -Wall -W ${WERROR} -DUTI_DEBUG_ON $AM_CPPFLAGS")

AM_DEP_TRACK

AC_CONFIG_FILES([Makefile \
                 src/Makefile \
                 src/tests/Makefile \
                 src/pymodule/Makefile \
                 src/pymodule/tests/Makefile \
                 opensand_conf.pc
                 ])

AC_OUTPUT

