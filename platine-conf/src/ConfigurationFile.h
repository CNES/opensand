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
 * @file ConfigurationFile.h
 * @brief Reading parameters from a configuration file
 * @author Viveris Technologies
 */

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <string>
#include <map>
#include <iostream>

using namespace std;

/** unused macro to avoid compilation warning with unused parameters */
#ifdef __GNUC__
#  define UNUSED(x) x __attribute__((unused))
#elif __LCLINT__
#  define UNUSED(x) /*@unused@*/ x
#else               /* !__GNUC__ && !__LCLINT__ */
#  define UNUSED(x) x
#endif              /* !__GNUC__ && !__LCLINT__ */

// Longueur max d'une ligne du fichier de config
// max length of a line inside the config file
#define CONF_LINE_MAX 512

#define CONF_SECTION_BEGIN  '['
#define CONF_SECTION_END    ']'
#define CONF_COMMENT        '#'  // character used for comments
#define CONF_AFFECTATION    '='

#define CONF_DEFAULT_FILE  "/etc/platine/core.conf"


/*
 * @class ConfigurationFile
 * @brief Reading parameters from a configuration file
 *
 * At startup, the whole configuration file content is loaded in memory
 * On msg_init event, each bloc gets its parameters from the config\n
 *
 * Sections format:  [sectionName]\n
 * Comments: lines beginning with '#' are ignored\n\n
 * 3 types of entries are supported inside a section:\n
 * - stringKey=stringValue   ex:  netInterface=eth0
 * - intKey=intValue         ex:  carrierId=2
 * - list item: a line without any '=' sign. Ex:\n
 *   <tt> # ClassId  ClassName\n
 *             1       voice\n
 *             2       ftp</tt>
 *
 */
class ConfigurationFile
{
 public:
	// Ctor & dtor
	ConfigurationFile(void);
	virtual ~ConfigurationFile(void);

	// Load/unload the whole configuration file content into/from memory
	int loadConfig(const string confFileName);
	void unloadConfig();

	// Get a string or integer value
	int getStringValue (const char *section, const char *key, string &value);
	int getIntegerValue(const char *section, const char *key, int &value);
	int getLongIntegerValue(const char *section, const char *key, long &value);

	// Get the number of items in the list; Get an item from the list
	int getNbListItems(const char *section);
	int getListItem(const char *section, unsigned short itemIdx, string &lineValue);

 private:
	// read a line from file
	int readLine(char **ptrBuffer, string &section, string &key, string &value);

	// Delete spaces and tabulations at beginning and end of string
	int supprSpaces(char *str);

	/// Association table between keys and values
	typedef map<string,string> keyItems_t;
	/// Association table between list item index and line value
	typedef map<unsigned char,string> listItems_t;
	/// Section content: list of (key,value) and list of (idx, line value)
	typedef struct
	{
		keyItems_t  keyItems;
		unsigned short nbListItems;
		listItems_t listItems;
	} entries_t;

	/// Association table between section name and content
	typedef map<string, entries_t *> sectionEntries_t;
	/// Contains all the sections of the file associated to their content
	sectionEntries_t sectionEntries;
};


// Configuration file content is loaded in this object at main initialization
//TODO remove that
extern ConfigurationFile globalConfig;

// Transform value "y" or "Y" in true; else false
#define CONF_VALUE_YES(val) (strncasecmp(val, "y", 1)==0)


#endif /* CONFIGURATION_H */
