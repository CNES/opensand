/**
 * @file uti_debug.cpp
 * @brief Manage debug levels of all packages
 * @author AQL (sylvie)
 */

#include <stdio.h>
#include <signal.h>
#include <string.h>


#include "uti_debug.h"
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
	int ret, level;

	for(i = 0; i < PKGINFO_NB; i++)
	{
		ret = globalConfig.getIntegerValue(SECTION_DEBUG, pkgInfo[i].name, level);
		if(!ret)
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
