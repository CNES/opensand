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

	// Get a value with format string, int, long or bool
    bool getValue(const char *section, const char *key, string &value);
    bool getValue(const char *section, const char *key, int &value);
    bool getValue(const char *section, const char *key, long &value);
    bool getValue(const char *section, const char *key, bool &value);

	// Get the number of items in the list; Get the items from the list
	bool getNbListItems(const char *section, const char *key, int &value);
	bool getListItems(const char *section, const char *key, ConfigurationList &list);

	// Get a value from a list attribute with format string, int, long or bool
	bool getAttributeValue(ConfigurationList::iterator iter,
	                       const char *attribute,
	                       string &value);
	bool getAttributeValue(ConfigurationList::iterator iter,
	                       const char *attribute,
	                       int &value);
	bool getAttributeValue(ConfigurationList::iterator iter,
	                       const char *attribute,
	                       long &value);
	bool getAttributeValue(ConfigurationList::iterator iter,
	                       const char *attribute,
	                       bool &value);

	// Get a string or integer from a line in a list
	bool getValueInList(ConfigurationList list,
	                    const char *id,
	                    const string id_val,
	                    const char *attribute,
	                    string &value);
	bool getValueInList(const char *section,
	                    const char *key,
	                    const char *id,
	                    const string id_val,
	                    const char *attribute,
	                    string &value);
	bool getValueInList(ConfigurationList list,
	                    const char *id,
	                    const string id_val,
	                    const char *attribute,
	                    int &value);
	bool getValueInList(const char *section,
	                    const char *key,
	                    const char *id,
	                    const string id_val,
	                    const char *attribute,
	                    int &value);
	bool getValueInList(ConfigurationList list,
	                    const char *id,
	                    const string id_val,
	                    const char *attribute,
	                    long &value);
	bool getValueInList(const char *section,
	                    const char *key,
	                    const char *id,
	                    const string id_val,
	                    const char *attribute,
	                    long &value);
	bool getValueInList(ConfigurationList list,
	                    const char *id,
	                    const string id_val,
	                    const char *attribute,
	                    bool &value);
	bool getValueInList(const char *section,
	                    const char *key,
	                    const char *id,
	                    const string id_val,
	                    const char *attribute,
	                    bool &value);

 private:
	/// a vector of XML DOM parsers
	vector<xmlpp::DomParser *> parsers;

	/// get a section node in XML configuration file
	bool getSection(const char *section,
	                xmlpp::Node::NodeList &sectionList);
	/// get a key node in XML configuration file
	bool getKey(const char *section,const char*key,
	            const xmlpp::Element **keyNode);

	/// generic functions to get value
	bool getStringValue(const char *section, const char *key, string &value);
	template <class T>
		bool getValueTemplate(const char *section, const char *key, T &value);

	/// generic functions to get value in list
	bool getAttributeStringValue(ConfigurationList::iterator iter,
	                             const char *attribute,
	                             string &value);
	template <class T>
		bool getAttributeValueTemplate(ConfigurationList::iterator iter,
		                               const char *attribute,
		                               T &value);
	/// generic functions to get value in list from elements
	bool getStringValueInList(ConfigurationList list,
	                          const char *id,
	                          const string id_val,
	                          const char *attribute,
	                          string &value);
	template <class T>
		bool getValueInListTemplate(ConfigurationList list,
		                             const char *id,
		                             const string id_val,
		                             const char *attribute,
		                             T &value);
	bool getStringValueInList(const char *section,
	                          const char *key,
	                          const char *id,
	                          const string id_val,
	                          const char *attribute,
	                          string &value);
	template <class T>
		bool getValueInListTemplate(const char *section,
		                            const char *key,
		                            const char *id,
		                            const string id_val,
		                            const char *attribute,
		                            T &value);
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


#endif /* CONFIGURATION_H */
