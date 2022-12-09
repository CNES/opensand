#!/bin/bash

# Process this file with autoconf to produce a configure script.
AC_INIT(opensand-core, 6.2.0, [http://opensand.org])
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
PKG_PROG_PKG_CONFIG
CXXFLAGS="$CXXFLAGS -std=c++17"


# check for opensand-conf library
PKG_CHECK_MODULES([OPENSAND_CONF], [opensand_conf = 6.2.0])
LIBS="$OPENSAND_CONF_LIBS $LIBS"
AC_SUBST(AM_CPPFLAGS, "$OPENSAND_CONF_CFLAGS $AM_CPPFLAGS")

# check for opensand-output library
PKG_CHECK_MODULES([OPENSAND_OUTPUT], [opensand_output = 6.2.0])
LIBS="$OPENSAND_OUTPUT_LIBS $LIBS"
AC_SUBST(AM_CPPFLAGS, "$OPENSAND_OUTPUT_CFLAGS $AM_CPPFLAGS")

# check for opensand-rt library
PKG_CHECK_MODULES([OPENSAND_RT], [opensand_rt = 6.2.0])
LIBS="$OPENSAND_RT_LIBS $LIBS"
AC_SUBST(AM_CPPFLAGS, "$OPENSAND_RT_CFLAGS $AM_CPPFLAGS")

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

AC_SUBST(AM_CPPFLAGS, "$AM_CPPFLAGS -g -Wall -Wextra ${WERROR} -DUTI_DEBUG_ON")

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
	src/conf/Makefile \
	src/dvb/Makefile \
	src/dvb/utils/Makefile \
	src/dvb/ncc_interface/Makefile \
	src/dvb/fmt/Makefile \
	src/dvb/dama/Makefile \
	src/dvb/saloha/Makefile \
	src/dvb/core/Makefile \
	src/encap/Makefile \
	src/lan_adaptation/Makefile \
	src/interconnect/Makefile \
	src/sat_carrier/Makefile \
	src/sat_carrier/tests/Makefile \
	src/physical_layer/Makefile \
	src/physical_layer/tests/Makefile \
	src/physical_layer/plugins/Makefile \
	src/physical_layer/plugins/attenuation/Makefile \
	src/physical_layer/plugins/attenuation/on_off/Makefile \
	src/physical_layer/plugins/attenuation/ideal/Makefile \
	src/physical_layer/plugins/attenuation/file/Makefile \
	src/physical_layer/plugins/attenuation/triangular/Makefile \
	src/physical_layer/plugins/error_insertion/Makefile \
	src/physical_layer/plugins/error_insertion/gate/Makefile \
	src/physical_layer/plugins/minimal_condition/Makefile \
	src/physical_layer/plugins/minimal_condition/acm_loop/Makefile \
	src/physical_layer/plugins/minimal_condition/constant/Makefile \
	src/physical_layer/plugins/satdelay/Makefile \
	src/physical_layer/plugins/satdelay/constant/Makefile \
	src/physical_layer/plugins/satdelay/file/Makefile \
	src/sat/Makefile \
	src/system/Makefile \
	opensand_plugin.pc \
	doc/doxygen \
])

AC_OUTPUT

