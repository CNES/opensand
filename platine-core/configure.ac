#!/bin/bash

# Process this file with autoconf to produce a configure script.
AC_INIT(platine-core, 3.0.0, Viveris Technologies)
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


# check for ROHC library presence if enabled

# check pkg-config presence
AC_CHECK_PROG(pkg_config, pkg-config, [yes], [AC_MSG_ERROR([pkg-config not found!])])


# check for ROHC library path
PKG_CHECK_MODULES([PLATINE_ROHC], [rohc >= 1.3.1])
LIBS="$PLATINE_ROHC_LIBS $LIBS"
CFLAGS="$PLATINE_ROHC_CFLAGS $CFLAGS"

# check for gse library
PKG_CHECK_MODULES([PLATINE_GSE], [gse >= 1.0.0])
LIBS="$PLATINE_GSE_LIBS $LIBS"
CFLAGS="$PLATINE_GSE_CFLAGS $CFLAGS"

# check for platine-conf library
PKG_CHECK_MODULES([PLATINE_CONF], [platine_conf >= 3.0.0])
LIBS="$PLATINE_CONF_LIBS $LIBS"
CFLAGS="$PLATINE_CONF_CFLAGS $CFLAGS"

# check for platine-env-plane library
PKG_CHECK_MODULES([PLATINE_ENV_PLANE], [platine_env_plane >= 3.0.0])
LIBS="$PLATINE_ENV_PLANE_LIBS $LIBS"
CFLAGS="$PLATINE_ENV_PLANE_CFLAGS $CFLAGS"

# check for platine-margouilla library
PKG_CHECK_MODULES([PLATINE_MARGOUILLA], [platine_margouilla >= 3.0.0])
LIBS="$PLATINE_MARGOUILLA_LIBS $LIBS"
CFLAGS="$PLATINE_MARGOUILLA_CFLAGS $CFLAGS"

#check if Ule security is enable
AC_ARG_ENABLE(ule_security, [  --enable-ule-security   enable ULE security  [[default=no]]], enable_ule_security=$enableval, enable_ule_security=no)
AM_CONDITIONAL(ule_security, [test "$enable_ule_security" = "yes"])
if test "$enable_ule_security" = "yes" ; then
  AC_CHECK_HEADERS([cryptopp/arc4.h], [], [AC_MSG_ERROR([cryptopp/arc4.h header not found!])])
  AC_CHECK_LIB(cryptopp, ARC4, [], [AC_MSG_ERROR([symbol ARC4 not found in libcryptopp!])])
fi

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


# Install binaries and libraries in usr/bin
AC_PREFIX_DEFAULT("/usr")

AM_DEP_TRACK

platine_root_dir=$(pwd)
platine_doc_dir=${platine_root_dir}/doc

AC_SUBST(platine_root_dir)
AC_SUBST(platine_doc_dir)

AC_CONFIG_FILES([ \
	Makefile \
	src/Makefile \
	src/common/Makefile \
	src/common/tests/Makefile \
	src/dvb_rcs/Makefile \
	src/dvb_rcs/utils/Makefile \
	src/dvb_rcs/switch/Makefile \
	src/dvb_rcs/pep/Makefile \
	src/dvb_rcs/dra/Makefile \
	src/dvb_rcs/modcod/Makefile \
	src/dvb_rcs/dama/Makefile \
	src/dvb_rcs/core/Makefile \
	src/encap/Makefile \
	src/qos_data/Makefile \
	src/sat_carrier/Makefile \
	src/system/Makefile \
	src/tun_interface/Makefile \
	doc/doxygen \
])

AC_OUTPUT


