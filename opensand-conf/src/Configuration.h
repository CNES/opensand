/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
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
 * @file Configuration.h
 * @brief GLobal interface for configuration file reading
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 */

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <string>

#include "ConfigurationList.h"
#include "ConfigurationFile.h"

using namespace std;

/** unused macro to avoid compilation warning with unused parameters */
#ifdef __GNUC__
#  define UNUSED(x) x __attribute__((unused))
#elif __LCLINT__
#  define UNUSED(x) /*@unused@*/ x
#else               /* !__GNUC__ && !__LCLINT__ */
#  define UNUSED(x) x
#endif              /* !__GNUC__ && !__LCLINT__ */

#define CONF_TOPOLOGY      "/etc/opensand/topology.conf"
#define CONF_GLOBAL_FILE   "/etc/opensand/core_global.conf"
#define CONF_DEFAULT_FILE  "/etc/opensand/core.conf"

/*
 * @class Conf
 * @brief GLobal interface for configuration file reading
 *
 * At startup, the whole configuration files contents are loaded in memory
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
 *   <!-- section with spots -->
 *   <section>
 *     <spot id="SPOT_ID1">
 *       <!-- table in spot -->
 *       <table>
 *         <line param1="val1" param2="val2" />
 *       </table>
 *       <!-- key in spot -->
 *       <key>val</key>
 *     </spot>
 *     <spot id="SPOT_ID2">
 *       <!-- table in spot -->
 *       <table>
 *         <line param1="val1" param2="val2" />
 *       </table>
 *       <!-- key in spot -->
 *       <key>val</key>
 *      </spot>
 *    </section>
 *  </configuration>
 */
class Conf
{
 public:

	Conf(void);
	~Conf(void);
	
	/**
	 * map between section name and ConfigurationList of section
	 */ 
	static map<string, ConfigurationList> section_map;
	
	/**
	 * Load the whole configuration file content into memory
	 * @param conf_file the configuration file path
	 * @return  true on success, false otherwise
	 */
	static bool loadConfig(const string conf_file);

	/**
	 * Load some configuration files content into memory
	 * @param conf_files the configuration files path
	 * @return  true on success, false otherwise
	 */
	static bool loadConfig(const vector<string> conf_files);

	/**
	 * @brief Get the component among sat, gw, st or ws
	 *
	 * @param compo  OUT: the component type
	 * @return true if an adequate component was found, false otherwise
	 */
	static bool getComponent(string &compo);

	/**
	 * Read a value from configuration
	 *
	 * @param  section  the section
	 * @param  key      name of the key
	 * @param  value    the value
	 * @return  true on success, false otherwise
	 */
	template <class T>
	static bool getValue(ConfigurationList section, const char *key, T &val);

	/**
	 * Read a value from configuration
	 *
	 * @param  iter     the iterator
	 * @param  value    the value
	 * @return  true on success, false otherwise
	 */
	/* TODO is this one used !!?? */
	template <class T>
	static bool getValue(ConfigurationList::iterator iter, 
	                     T &val);

	/**
	 * Get the section node list
	 * @param  sectionList section list
	 * @param  key         node name
	 * @param  nodeList    node list
	 * @return true on success, false otherwise
	 */
	static bool getListNode(ConfigurationList sectionList,
                            const char *key,
                            xmlpp::Node::NodeList &nodeList);
	
	/**
	 * get the element from the list with attribute value
	 * @param  list             the origal element list
	 * @param  attribute_name   the attribute name
	 * @param  attribute_value  the attribute value
	 * @param  elements         the list of found elements
	 * @return true on success and false otherwise
	 */
	template <class T>
	static bool getElementWithAttributeValue(ConfigurationList list,
                                             const char *attribute_name,
                                             T &attribute_value,
                                             ConfigurationList &elements);

	/**
	 * Read the number of elements in a list
	 *
	 * @param  section  name of the section
	 * @param  key      name of the list key
	 * @param  nbr      the number of elements in the list
	 * @return  true on success, false otherwise
	 */
	static bool getNbListItems(ConfigurationList section, 
	                           const char *key, 
	                           int &value);

	/**
	 * Read the number of elements in a list
	 *
	 * @param  section  the section
	 * @param  key      name of the list key
	 * @param  nbr      the number of elements in the list
	 * @return  true on success, false otherwise
	 */
	static bool getNbListItems(ConfigurationList section, 
	                           const char *key, 
	                           unsigned int &value);

	/**
	 * Get the elements from the list
	 *
	 * @param  node     the node
	 * @param  key      name of the list key
	 * @param  list     the list
	 * @return  true on success, false otherwise
	 */
	static bool getListItems(xmlpp::Node *node, 
	                         const char *key, 
	                         ConfigurationList &list);

	/**
	 * Get the elements from the list
	 *
	 * @param  section  the section
	 * @param  key      name of the list key
	 * @param  list     the list
	 * @return  true on success, false otherwise
	 */
	static bool getListItems(ConfigurationList section, 
	                         const char *key, 
	                         ConfigurationList &list);

	/**
	 * Get the value of an attribute in a list element
	 *
	 * @param  elt        an iterator on a ConfigurationList
	 * @param  attribute  the attribute name
	 * @param  value      attribute value
	 * @return  true on success, false otherwise
	 */
	template <class T>
	static bool getAttributeValue(ConfigurationList::iterator iter,
	                              const char *attribute,
	                              T &value);

	/**
	 * Get a value from a list element identified by an attribute value
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
	static bool getValueInList(ConfigurationList section,
	                           const char *key,
	                           const char *id,
	                           const string id_val,
	                           const char *attribute,
	                           T &value);

	/**
	 * Get a value from a list element identified by an attribute value
	 *
	 * @param  list      the list
	 * @param  id        the reference attribute
	 * @param  id_val    the reference attribute value
	 * @param  attribute the desired attribute
	 * @param  value     the desired value
	 * @return  true on success, false otherwise
	 */
	template <class T>
	static bool getValueInList(ConfigurationList list,
	                           const char *id,
	                           const string id_val,
	                           const char *attribute,
	                           T &value);
	
	
	/**
	 * Load the log desired display levels
	 * 
	 * @param levels    OUT: the log levels
	 * @param specific  OUT: specific log levels
	 * @return true on success, false otherwise
	 */
	static bool loadLevels(map<string, log_level_t> &levels,
	                       map<string, log_level_t> &specific);

 private:

	static ConfigurationFile global_config;

	static void loadMap(void);
};



template <class T>
bool Conf::getValue(ConfigurationList section, const char *key, T &val)
{
	return Conf::global_config.getValue(section, key, val);
}

template <class T>
bool Conf::getValue(ConfigurationList::iterator iter, T &val)
{
	return Conf::global_config.getValue(iter, val);
}


template <class T>
bool Conf::getAttributeValue(ConfigurationList::iterator iter,
                             const char *attribute,
                             T &value)
{
	return Conf::global_config.getAttributeValue(iter, attribute, value);
}

template <class T>
bool Conf::getElementWithAttributeValue(ConfigurationList list,
                                  const char *attribute_name,
                                  T &attribute_value,
                                  ConfigurationList &elements)
{
	return Conf::global_config.getElementWithAttributeValue(list,
	                                                        attribute_name,
	                                                        attribute_value,
	                                                        elements);
}


template <class T>
bool Conf::getValueInList(ConfigurationList list,
                          const char *id,
                          const string id_val,
                          const char *attribute,
                          T &value)
{
	return Conf::global_config.getValueInList(list, id, id_val,
	                                          attribute, value);
}


template <class T>
bool Conf::getValueInList(ConfigurationList section,
                          const char *key,
                          const char *id,
                          const string id_val,
                          const char *attribute,
                          T &value)
{
	return Conf::global_config.getValueInList(section, key, id, id_val,
	                                          attribute, value);
}


#endif
