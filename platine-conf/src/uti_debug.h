/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file uti_debug.h
 * @brief Debug macros definition
 * @author Viveris Technologies
 *
 * A debug level is associated to each package.
 * A source file indicates to which package it belongs by declaring the
 * DBG_PACKAGE macro before including uti_debug.h, e.g. for BlocIPQoS:\n
 * <tt>\#define DBG_PACKAGE PKG_QOS_DATA\n
 *     \#include "uti_debug.h"</tt>\n
 * if DBG_PACKAGE macro is not defined, file will belong to the default package
 *
 * The debug levels are configurable dynamically:
 * - all levels are statically initialized to 0
 * - on startup, levels are read from section [Debug] of the configuration
 *   file; each line has format:\n
 *   \<package name\>=\<package debug level\>, where name is exactly the pkg macro,
 *   e.g., for message interface, msg_interface=2
 * - during execution, one can modify conf file and send a SIGUSR1 signal to
 *   the process (kill -10 \<pid\>), to force reload of all levels
 * - in case the package level is not defined in the conf file, the package
 *   can override the default 0 value by calling UTI_DEBUG_LEVEL(level)
 *
 */

/**
 * @def UTI_ERROR(fmt, args...)
 * displays an error message, which should be notified to the user,
 * even in a release version.
 * ALWAYS generated, independently of UTI_DEBUG_ON macro and of debug level.
 */
/**
 * @def UTI_NOTICE(fmt, args...)
 * displays a significative informational message
 * Generated independently of debug level but when UTI_DEBUG_ON macro is defined
 */
/**
 * @def UTI_INFO(fmt, args...)
 * displays a global level informational message:
 * - is defined only if UTI_DEBUG_ON macro defined, otherwise empty
 * - message is displayed if package level >= 1
 */
/**
 * @def UTI_DEBUG(fmt, args...)
 * displays a detailed level informational message:
 * - is defined only if UTI_DEBUG_ON macro defined, otherwise empty
 * - message is displayed if package level >= 2
 */
/*
 * @def UTI_DEBUG_L3(fmt, arg...)
 * display a very detailed level informational message (inner loop debugging)
 * - is defined only if UTI_DEBUG_ON macro is defined, otherwise empty
 * - message is displayed if package level >= 3
 */
/**
 * @def UTI_DUMP(label, data, len)
 * dumps in hexa the data buffer:
 * - is defined only if UTI_DEBUG_ON macro defined, otherwise empty
 * - message is displayed if package level >= 2
 */

/**
 * @def UTI_DEBUG_LEVEL(level)
 * sets the package debug level to level argument, only if level was
 * not read from conf file
 */

#ifndef _UTI_DEBUG_H_
#define _UTI_DEBUG_H_

#include "syslog.h"

// Each package has its own level of debug
#define PKG_DEFAULT       default
#define PKG_DAMA_DC       dama_dc
#define PKG_DAMA_DA       dama_da
#define PKG_DVB_RCS       dvb_rcs
#define PKG_DVB_RCS_TAL   dvb_rcs_tal
#define PKG_DVB_RCS_NCC   dvb_rcs_ncc
#define PKG_DVB_RCS_SAT   dvb_rcs_sat
#define PKG_QOS_DATA      qos_data
#define PKG_SAT_CARRIER   sat_carrier
#define PKG_ENCAP         encap

#ifndef DBG_PACKAGE
#define DBG_PACKAGE  PKG_DEFAULT
#endif

#define dbgLevel_(x) dbgLevel_ ## x
#define dbgLevel(x) dbgLevel_(x)

extern unsigned char dbgLevel(DBG_PACKAGE);

#define dbgPkgStr_(x) #x
#define dbgPkgStr(x) dbgPkgStr_(x)
#define dbgHeader(x) (const char*) "[" dbgPkgStr(x) "]"


/** Print a trace unconditionally */
#define UTI_PRINT(level, fmt, args...)                      \
	do                                                        \
	{                                                         \
		syslog(level, dbgHeader(DBG_PACKAGE)                    \
		       " [%s:%s():%d] " fmt,                            \
		       __FILE__, __FUNCTION__, __LINE__, ##args);       \
	}                                                         \
	while(0)


/** Print an error trace */
#define UTI_ERROR(fmt, args...)                             \
	do                                                        \
	{                                                         \
		UTI_PRINT(LOG_ERR, "ERR: " fmt, ##args);                \
	}                                                         \
	while(0)


/** Print a notice trace */
#define UTI_NOTICE(fmt, args...)                            \
	do                                                        \
	{                                                         \
		UTI_PRINT(LOG_NOTICE, fmt, ##args);                     \
	}                                                         \
	while(0)


/** Print a information trace */
#define UTI_INFO(fmt, args...)                              \
	do                                                        \
	{                                                         \
		if(dbgLevel(DBG_PACKAGE) >= 1)                          \
		{                                                       \
			UTI_PRINT(LOG_INFO, fmt, ##args);                     \
		}                                                       \
	}                                                         \
  while(0)


/** Print a trace if started in normal debug mode */
#define UTI_DEBUG(fmt, args...)                             \
	do                                                        \
	{                                                         \
		if(dbgLevel(DBG_PACKAGE) >= 2)                          \
		{                                                       \
			UTI_PRINT(LOG_DEBUG, fmt, ##args);                    \
		}                                                       \
	}                                                         \
	while(0)


/** Print a trace if started in verbose debug mode */
#define UTI_DEBUG_L3(fmt, args...)                          \
	do                                                        \
	{                                                         \
		if(dbgLevel(DBG_PACKAGE) >= 3)                          \
		{                                                       \
			UTI_PRINT(LOG_DEBUG, fmt, ##args);                    \
		}                                                       \
	}                                                         \
	while(0)


/** Dump data area for debug purpose */
#define UTI_DUMP(label, data, len) \
{ \
	if(dbgLevel(DBG_PACKAGE)>=2) \
	{ \
		string str=label; char x[4]; int i; \
		for(i=0;i<len;i++) \
		{ \
			sprintf(x, "%02X ", ((unsigned char *)data)[i]); \
			str += x; \
		} \
		syslog(LOG_DEBUG, dbgHeader(DBG_PACKAGE) "%s\n", str.c_str()); \
}}

#define UTI_DEBUG_LEVEL(level) UTI_setDefaultLevel(dbgPkgStr(DBG_PACKAGE),level)


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define HERE() \
	std::string(__FILE__) + ":" + \
	TOSTRING(__LINE__) + " " + \
	__FUNCTION__ + "()"

extern void UTI_readDebugLevels();
extern void UTI_setDefaultLevel(const char *pkg, unsigned char level);

#endif // _UTI_DEBUG_H_
