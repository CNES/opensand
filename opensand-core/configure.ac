#!/bin/bash

# Process this file with autoconf to produce a configure script.
AC_INIT(opensand-core, trunk, [http://opensand.org])
AC_CONFIG_SRCDIR([src])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

#
# defines the required versions of libraries
#

# By default, generate static libraries
#AC_DISABLE_SHARED
#AC_DISABLE_STATIC

# Checks for programs.programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for libraries.
AC_SUBST(SET_MAKE,MAKE=make)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h malloc.h netdb.h netinet/in.h stddef.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h syslog.h unistd.h])

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


# check pkg-config presence
AC_CHECK_PROG(pkg_config, pkg-config, [yes], [AC_MSG_ERROR([pkg-config not found!])])


# check for opensand-conf library
PKG_CHECK_MODULES([OPENSAND_CONF], [opensand_conf = trunk])
LIBS="$OPENSAND_CONF_LIBS $LIBS"
AC_SUBST(AM_CPPFLAGS, "$OPENSAND_CONF_CFLAGS $AM_CPPFLAGS")

# check for opensand-env-plane library
PKG_CHECK_MODULES([OPENSAND_ENV_PLANE], [opensand_env_plane = trunk])
LIBS="$OPENSAND_ENV_PLANE_LIBS $LIBS"
AC_SUBST(AM_CPPFLAGS, "$OPENSAND_ENV_PLANE_CFLAGS $AM_CPPFLAGS")

# check for opensand-margouilla library
PKG_CHECK_MODULES([OPENSAND_MARGOUILLA], [opensand_margouilla = trunk])
LIBS="$OPENSAND_MARGOUILLA_LIBS $LIBS"
AC_SUBST(AM_CPPFLAGS, "$OPENSAND_MARGOUILLA_CFLAGS $AM_CPPFLAGS")


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

AC_SUBST(AM_CPPFLAGS, "$AM_CPPFLAGS -g -Wall ${WERROR} -DUTI_DEBUG_ON")

# Install binaries and libraries in usr/bin
#AC_PREFIX_DEFAULT("/usr")

AM_DEP_TRACK

opensand_root_dir=$(pwd)
opensand_doc_dir=${opensand_root_dir}/doc

AC_SUBST(opensand_root_dir)
AC_SUBST(opensand_doc_dir)

AC_CONFIG_FILES([ \
	Makefile \
	src/Makefile \
	src/common/Makefile \
	src/common/tests/Makefile \
	src/dvb_rcs/Makefile \
	src/dvb_rcs/utils/Makefile \
	src/dvb_rcs/pep/Makefile \
	src/dvb_rcs/dra/Makefile \
	src/dvb_rcs/modcod/Makefile \
	src/dvb_rcs/dama/Makefile \
	src/dvb_rcs/switch/Makefile \
	src/dvb_rcs/core/Makefile \
	src/encap/Makefile \
	src/qos_data/Makefile \
	src/sat_carrier/Makefile \
	src/physical_layer/Makefile \
	src/system/Makefile \
	src/tun_interface/Makefile \
	conf/Makefile \
	opensand_plugin.pc \
	doc/doxygen \
])

AC_OUTPUT


