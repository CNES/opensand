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
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 */

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <string>
#include <map>
#include <iostream>
#include <libxml++/libxml++.h>

#include "ConfigurationList.h"

using namespace std;

/** unused macro to avoid compilation warning with unused parameters */
#ifdef __GNUC__
#  define UNUSED(x) x __attribute__((unused))
#elif __LCLINT__
#  define UNUSED(x) /*@unused@*/ x
#else               /* !__GNUC__ && !__LCLINT__ */
#  define UNUSED(x) x
#endif              /* !__GNUC__ && !__LCLINT__ */

#define CONF_GLOBAL_FILE  "/etc/platine/core_global.conf"
#define CONF_DEFAULT_FILE  "/etc/platine/core.conf"


/*
 * @class ConfigurationFile
 * @brief Reading parameters from a configuration file
 *
 * At startup, the whole configuration files contents are loaded in memory
 * On msg_init event, each bloc gets its parameters from the config\n
 *
 * XML format:
 * <?xml version="1.0" encoding="UTF-8"?>
 * <configuration component='compo'>
 *   <!-- section description -->
 *   <section>
 *     <!-- table and parameters description -->
 *     <table>
 *       <line param1="val1" param2="val2" />
 *     </table>
 *     <!-- key description -->
 *     <key>val</key>
 *    </section>
 *  </configuration>
 */
class ConfigurationFile
{
 public:
	// Ctor & dtor
	ConfigurationFile(void);
	virtual ~ConfigurationFile(void);

	// Load/unload the whole configuration file content into/from memory
	bool loadConfig(const string confFileName);
	void unloadConfig();

	// Get a string or integer value
	bool getStringValue(const char *section, const char *key, string &value);
	bool getIntegerValue(const char *section, const char *key, int &value);
	bool getLongIntegerValue(const char *section, const char *key, long &value);

	// Get the number of items in the list; Get the items from the list
	bool getNbListItems(const char *section, const char *key, int &value);
	bool getListItems(const char *section, const char *key, ConfigurationList &list);

	// Get a string or integer value from a list attribute
	bool getAttributeStringValue(ConfigurationList::iterator iter,
	                             const char *attribute,
	                             string &value);
	bool getAttributeIntegerValue(ConfigurationList::iterator iter,
	                              const char *attribute,
	                              int &value);
	bool getAttributeLongIntegerValue(ConfigurationList::iterator iter,
	                                  const char *attribute,
	                                  long &value);

	// Get a string or integer from a line in a list
	bool getStringValueInList(ConfigurationList list,
	                          const char *id,
	                          const string id_val,
	                          const char *attribute,
	                          string &value);
	bool getStringValueInList(const char *section,
	                          const char *key,
	                          const char *id,
	                          const string id_val,
	                          const char *attribute,
	                          string &value);
	bool getIntegerValueInList(ConfigurationList list,
	                           const char *id,
	                           const string id_val,
	                           const char *attribute,
	                           int &value);
	bool getIntegerValueInList(const char *section,
	                           const char *key,
	                           const char *id,
	                           const string id_val,
	                           const char *attribute,
	                           int &value);
	bool getLongIntegerValueInList(ConfigurationList list,
	                               const char *id,
	                               const string id_val,
	                               const char *attribute,
	                               long int &value);
	bool getLongIntegerValueInList(const char *section,
	                               const char *key,
	                               const char *id,
	                               const string id_val,
	                               const char *attribute,
	                               long &value);

 private:
	/// a vector of XML DOM parsers
	vector<xmlpp::DomParser *> _parsers;

	/// get a section node in XML configuration file
	bool getSection(const char *section,
	                xmlpp::Node::NodeList &sectionList);
	/// get a key node in XML configuration file
	bool getKey(const char *section,const char*key,
	            const xmlpp::Element **keyNode);
};


inline string toString(int val)
{
	stringstream str;
	str << val;

	return str.str();
}

inline string toString(long val)
{
	stringstream str;
	str << val;

	return str.str();
}

// Configuration file content is loaded in this object at main initialization
extern ConfigurationFile globalConfig;

// Transform value "y" or "Y" in true; else false
#define CONF_VALUE_YES(val) (((val == "y") ? 1 : 0) || \
                             ((val == "Y") ? 1 : 0) ||\
                             ((val == "true") ? 1 : 0) ||\
                             ((val == "True") ? 1 : 0) ||\
                             ((val == "1") ? 1 : 0))


#endif /* CONFIGURATION_H */
