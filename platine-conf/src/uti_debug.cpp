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
 * @file uti_debug.cpp
 * @brief Manage debug levels of all packages
 * @author Viveris Technologies
 */

#include <stdio.h>
#include <signal.h>
#include <string.h>


#include "uti_debug.h"
#include "conf.h"
#include "ConfigurationFile.h"

// All the package levels are defined and initialised here
// each package accesses its debug level with an extern
unsigned char dbgLevel(PKG_DEFAULT) = 0;
unsigned char dbgLevel(PKG_DAMA_DC) = 0;
unsigned char dbgLevel(PKG_DAMA_DA) = 0;
unsigned char dbgLevel(PKG_DVB_RCS) = 0;
unsigned char dbgLevel(PKG_DVB_RCS_TAL) = 0;
unsigned char dbgLevel(PKG_DVB_RCS_NCC) = 0;
unsigned char dbgLevel(PKG_DVB_RCS_SAT) = 0;
unsigned char dbgLevel(PKG_QOS_DATA) = 0;
unsigned char dbgLevel(PKG_SAT_CARRIER) = 0;
unsigned char dbgLevel(PKG_ENCAP) = 2;

/// List of packages handled by debug module
typedef struct
{
	const char *name;     ///< package name
	unsigned char *level; ///< ptr on debug level variable
	bool inConfig;        ///< true if pkg level read from conf file
} pkgInfo_t;

pkgInfo_t pkgInfo[] =
{
	{dbgPkgStr(PKG_DEFAULT), &dbgLevel(PKG_DEFAULT), false},
	{dbgPkgStr(PKG_DAMA_DC), &dbgLevel(PKG_DAMA_DC), false},
	{dbgPkgStr(PKG_DAMA_DA), &dbgLevel(PKG_DAMA_DA), false},
	{dbgPkgStr(PKG_DVB_RCS), &dbgLevel(PKG_DVB_RCS), false},
	{dbgPkgStr(PKG_DVB_RCS_SAT), &dbgLevel(PKG_DVB_RCS_SAT), false},
	{dbgPkgStr(PKG_DVB_RCS_NCC), &dbgLevel(PKG_DVB_RCS_NCC), false},
	{dbgPkgStr(PKG_DVB_RCS_TAL), &dbgLevel(PKG_DVB_RCS_TAL), false},
	{dbgPkgStr(PKG_QOS_DATA), &dbgLevel(PKG_QOS_DATA), false},
	{dbgPkgStr(PKG_SAT_CARRIER), &dbgLevel(PKG_SAT_CARRIER), false},
	{dbgPkgStr(PKG_ENCAP), &dbgLevel(PKG_ENCAP), false},
};

#define PKGINFO_NB sizeof(pkgInfo)/sizeof(pkgInfo_t)


/**
 * Dynamic change of debug levels on SIGUSR1 reception
 */
void reloadDbgLevels(int UNUSED(sig))
{
	globalConfig.unloadConfig();
	globalConfig.loadConfig(CONF_DEFAULT_FILE);
	UTI_readDebugLevels();
}

/**
 * Read debug levels of all packages from configuration file
 */
void UTI_readDebugLevels()
{
	unsigned int i;
	int level;

	for(i = 0; i < PKGINFO_NB; i++)
	{
		if(globalConfig.getValue(SECTION_DEBUG, pkgInfo[i].name, level))
		{
			*(pkgInfo[i].level) = (unsigned char) level;
			pkgInfo[i].inConfig = true;
		}
	}

	// Install an handler on SIGUSR1 to force debug levels reload
	signal(SIGUSR1, reloadDbgLevels);
}

/**
 * If pkg debug level not read from conf file, set it to level arg
 */
void UTI_setDefaultLevel(const char *pkg, unsigned char level)
{
	unsigned int i;

	for(i = 0; i < PKGINFO_NB; i++)
	{
		if(!strcmp(pkgInfo[i].name, pkg))
		{
			if(!pkgInfo[i].inConfig)
			{
				*(pkgInfo[i].level) = level;
			}
			break;
		}
	}
}
