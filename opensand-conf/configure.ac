# Process this file with autoconf to produce a configure script.
AC_INIT(opensand_conf, 0.9.0, [Viveris Technologies])
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

AC_SUBST([OPENSAND_CONF_REQ], ['libxml++-2.6 >= 2.10.0'])

PKG_CHECK_MODULES([LIBXMLXX], [$OPENSAND_CONF_REQ])
LIBS="$LIBXMLXX_LIBS $LIBS"

AC_SUBST(AM_CPPFLAGS, "$LIBXMLXX_CFLAGS")

# check for pkg-config
PKG_PROG_PKG_CONFIG

AC_SUBST(AM_CPPFLAGS, "-g -Wall -W -Werror -DUTI_DEBUG_ON $AM_CPPFLAGS")

AM_DEP_TRACK

AC_CONFIG_FILES([Makefile \
                 src/Makefile \
                 src/test/Makefile \
                 opensand_conf.pc
                 ])

AC_OUTPUT
