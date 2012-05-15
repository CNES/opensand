/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
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
#include <stdint.h>

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

#define CONF_TOPOLOGY      "/etc/platine/topology.conf"
#define CONF_GLOBAL_FILE   "/etc/platine/core_global.conf"
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

	/**
	 * Load the whole configuration file content into memory
	 * @param conf_file the configuration file path
	 * @return  true on success, false otherwise
	 */
	bool loadConfig(const string conf_file);
	/**
	 * Load some configuration files content into memory
	 * @param conf_files the configuration files path
	 * @return  true on success, false otherwise
	 */
	bool loadConfig(const vector<string> conf_files);
	void unloadConfig();

	/* Get a value */
	template <class T>
	bool getValue(const char *section, const char *key, T &val);

	// Get the number of items in the list; Get the items from the list
	bool getNbListItems(const char *section, const char *key, int &value);
	bool getListItems(const char *section, const char *key, ConfigurationList &list);

	// Get a value from a list attribute
	template <class T>
	bool getAttributeValue(ConfigurationList::iterator iter,
	                       const char *attribute,
	                       T &value);

	// Get a value from a line in a list
	template <class T>
	bool getValueInList(const char *section,
	                    const char *key,
	                    const char *id,
	                    const string id_val,
	                    const char *attribute,
	                    T &value);
	template <class T>
	bool getValueInList(ConfigurationList list,
	                    const char *id,
	                    const string id_val,
	                    const char *attribute,
	                    T &value);


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

	/// generic functions to get value in list
	bool getAttributeStringValue(ConfigurationList::iterator iter,
	                             const char *attribute,
	                             string &value);
	/// generic function to get value in list from elements
	bool getStringValueInList(ConfigurationList list,
	                          const char *id,
	                          const string id_val,
	                          const char *attribute,
	                          string &value);
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

// these functions should be in .h file because they are templates

/**
 * Read a value from configuration
 *
 * @param  section  name of the section
 * @param  key      name of the key
 * @param  value    the value
 * @return  true on success, false otherwise
 */
/* Get a value */
template <class T>
bool ConfigurationFile::getValue(const char *section, const char *key, T &val)
{
	string tmp_val;

	if(!this->getStringValue(section, key, tmp_val))
		return false;

	stringstream str(tmp_val);
	str >> val;
	if(str.fail())
	{
		return false;
	}

	return true;
}

/**
 * Get the value of an attribute in a list element
 *
 * @param  elt        an iterator on a ConfigurationList
 * @param  attribute  the attribute name
 * @param  value      attribute value
 * @return  true on success, false otherwise
 */
template <class T>
bool ConfigurationFile::getAttributeValue(ConfigurationList::iterator iter,
                                          const char *attribute,
                                          T &value)
{
	string tmp_val;

	if(!this->getAttributeStringValue(iter, attribute, tmp_val))
		return false;

	stringstream str(tmp_val);
	str >> value;
	if(str.fail())
	{
		return false;
	}

	return true;
}

/**
 * Get a value from a list element identified by a attribute value
 *
 * @param  list      the list
 * @param  id        the reference attribute
 * @param  id_val    the reference attribute value
 * @param  attribute the desired attribute
 * @param  value     the desired value
 * @return  true on success, false otherwise
 */
template <class T>
bool ConfigurationFile::getValueInList(ConfigurationList list,
                                       const char *id,
                                       const string id_val,
                                       const char *attribute,
                                       T &value)
{
	string tmp_val;

	if(!this->getStringValueInList(list, id, id_val, attribute, tmp_val))
		return false;

	stringstream str(tmp_val);
	str >> value;
	if(str.fail())
	{
		return false;
	}

	return true;
}

/**
 * Get a value from a list element identified by a attribute value
 *
 * @param  section   name of the section identifying the list
 * @param  key       name of the list key identifying the list
 * @param  id        the reference attribute
 * @param  id_val    the reference attribute value
 * @param  attribute the desired attribute
 * @param  value     the desired value
 * @return  true on success, false otherwise
 */
template <class T>
bool ConfigurationFile::getValueInList(const char *section,
                                       const char *key,
                                       const char *id,
                                       const string id_val,
                                       const char *attribute,
                                       T &value)
{
	ConfigurationList list;

	if(!this->getListItems(section, key, list))
	{
		goto error;
	}
	return this->getValueInList(list, id, id_val, attribute, value);

error:
	return false;
}


template <>
inline bool ConfigurationFile::getValue<bool>(const char *section,
                                              const char *key, bool &val)
{
	string tmp_val;

	if(!this->getValue<string>(section, key, tmp_val))
		return false;

	stringstream str(tmp_val);
	str >> std::boolalpha >> val;
	if(str.fail())
	{
		return false;
	}

	return true;
}

template <>
inline bool ConfigurationFile::getAttributeValue<bool>(ConfigurationList::iterator iter,
		                                               const char *attribute,
                                                       bool &value)
{
	string tmp_val;

	if(!this->getAttributeValue(iter, attribute, tmp_val))
		return false;

	stringstream str(tmp_val);
	str >> std::boolalpha >> value;
	if(str.fail())
	{
		return false;
	}

	return true;
}

/* only write this specialization because it will be called by the other one
 * and we should not surccharge a specialization */
template <>
inline bool ConfigurationFile::getValueInList<bool>(ConfigurationList list,
                                                    const char *id,
                                                    const string id_val,
                                                    const char *attribute,
                                                    bool &value)
{
	string tmp_val;

	if(!this->getValueInList(list, id, id_val, attribute, tmp_val))
		return false;

	stringstream str(tmp_val);
	str >> std::boolalpha >> value;
	if(str.fail())
	{
		return false;
	}

	return true;
}


template <>
inline bool ConfigurationFile::getValue<uint8_t>(const char *section,
                                                 const char *key, uint8_t &value)
{
	string tmp_val;
	unsigned int val;

	if(!this->getValue<string>(section, key, tmp_val))
		return false;

	stringstream str(tmp_val);
	str >> val;
	if(str.fail())
	{
		return false;
	}

	value = val;
	return true;
}

template <>
inline bool ConfigurationFile::getAttributeValue<uint8_t>(ConfigurationList::iterator iter,
		                                                 const char *attribute,
                                                         uint8_t &value)
{
	string tmp_val;
	unsigned int val;

	if(!this->getAttributeValue(iter, attribute, tmp_val))
		return false;

	stringstream str(tmp_val);
	str >> val;
	if(str.fail())
	{
		return false;
	}

	value = val;
	return true;
}

/* only write this specialization because it will be called by the other one
 * and we should not surccharge a specialization */
template <>
inline bool ConfigurationFile::getValueInList<uint8_t>(ConfigurationList list,
                                                    const char *id,
                                                    const string id_val,
                                                    const char *attribute,
                                                    uint8_t &value)
{
	string tmp_val;
	unsigned int val;

	if(!this->getValueInList(list, id, id_val, attribute, tmp_val))
		return false;

	stringstream str(tmp_val);
	str >> val;
	if(str.fail())
	{
		return false;
	}

	value = val;
	return true;
}



#endif /* CONFIGURATION_H */
