/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
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
 * @file ConfigurationFile.h
 * @brief Reading parameters from a configuration file
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @author Bénédicte MOTTO / <bmotto@toulouse.viveris.com>
 */

#ifndef CONFIGURATION_FILE_H
#define CONFIGURATION_FILE_H

#include <string>
#include <map>
#include <iostream>
#include <libxml++/libxml++.h>
#include <stdint.h>

#include "ConfigurationList.h"

#include <opensand_output/OutputLog.h>

using namespace std;


/*
 * @class ConfigurationFile
 * @brief Reading parameters from a configuration file
 *
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

	/**
	 * Unload the whole configuration file content from memory,
	 * i.e. free tables
	 */
	void unloadConfig();
	
	/**
	 * Create a Map with all section ConfigurationList and section name
	 * @param section_map the map between section name and configurationList
	 */
	void loadSectionMap(map<string, ConfigurationList> &section_map);
	
	/**
	 * @brief Get the component among sat, gw, st or ws
	 *
	 * @param compo  OUT: the component type
	 * @return true if an adequate component was found, false otherwise
	 */
	bool getComponent(string &compo);

	/**
	 * Read a value from configuration
	 *
	 * @param  section  the section
	 * @param  key      name of the key
	 * @param  value    the value
	 * @return  true on success, false otherwise
	 */
	template <class T>
	bool getValue(ConfigurationList section, const char *key, T &val);

	/**
	 * Read a value from configuration
	 *
	 * @param  iter     the iterator
	 * @param  value    the value
	 * @return  true on success, false otherwise
	 */
	template <class T>
	bool getValue(ConfigurationList::iterator iter, 
	              T &val);

	/**
	 * Get the section node list
	 * @param  sectionList section list
	 * @param  key         node name
	 * @param  nodeList    node list
	 * @return true on success, false otherwise
	 */
	bool getListNode(ConfigurationList sectionList,
                     const char *key,
                     xmlpp::Node::NodeList &nodeList);

	/**
	 * get the element from the list with attribute value
	 * @param  list             the origal element list
	 * @param  attribute_name   the attribute name
	 * @param  attribute_value  the attribute value
	 * @param  elements         the list of found elements
	 * @return true en success and false otherwise
	 */
	bool getElementWithAttributeStringValue(ConfigurationList list,
                                      const char *attribute_name,
                                      const char *attribute_value,
                                      ConfigurationList &elements);

	/**
	 * get the element from the list with attribute value
	 * @param  list             the origal element list
	 * @param  attribute_name   the attribute name
	 * @param  attribute_value  the attribute value
	 * @param  elements         the list of found elements
	 * @return true en success and false otherwise
	 */
	template <class T>
	bool getElementWithAttributeValue(ConfigurationList list,
                                      const char *attribute_name,
                                      T &attribute_value,
                                      ConfigurationList &elements);


	/**
	 * Read the number of elements in a list
	 *
	 * @param  section  the section
	 * @param  key      name of the list key
	 * @param  nbr      the number of elements in the list
	 * @return  true on success, false otherwise
	 */
	bool getNbListItems(ConfigurationList section, 
	                    const char *key, 
	                    int &value);

	/**
	 * Get the elements from the list
	 *
	 * @param  node  the node
	 * @param  key      name of the list key
	 * @param  list     the list
	 * @return  true on success, false otherwise
	 */
	bool getListItems(xmlpp::Node *node, 
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
	bool getListItems(ConfigurationList section, 
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
	bool getAttributeValue(ConfigurationList::iterator iter,
	                       const char *attribute,
	                       T &value);


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
	bool getValueInList(ConfigurationList section,
	                    const char *key,
	                    const char *id,
	                    const string id_val,
	                    const char *attribute,
	                    T &value);

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
	bool getValueInList(ConfigurationList list,
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
	bool loadLevels(map<string, log_level_t> &levels,
	                map<string, log_level_t> &specific);
	
 private:

	/// Output Log
	OutputLog *log_conf;

	/// a vector of XML DOM parsers
	vector<xmlpp::DomParser *> parsers;
	
	/**
	 * Get a XML section node from its name
	 *
	 * @param  section      name of the section
	 * @param  sectionNode  the XML section node
	 * @return  true on success, false otherwise
	 */
	bool getSection(const char *section,
	                xmlpp::Node::NodeList &sectionList);

	/**
	 * Get a XML key node from its name and its section name
	 *
	 * @param  section  name of the section
	 * @param  key      name of the key
	 * @param  keyNode  the XML key node
	 * @return  true on success, false otherwise
	 */
	bool getKey(ConfigurationList section,const char*key,
	            const xmlpp::Element **keyNode);

	/**
	 * Read a string value from configuration
	 *
	 * @param  section  name of the section
	 * @param  key      name of the key
	 * @param  value    value of the string
	 * @return  true on success, false otherwise
	 */
	bool getStringValue(ConfigurationList section, 
	                    const char *key, 
	                    string &value);

	/**
	 * Get the string value of an attribute in a list element
	 *
	 * @param  elt        an iterator on a ConfigurationList
	 * @param  attribute  the attribute name
	 * @param  value      attribute value
	 * @return  true on success, false otherwise
	 */
	bool getAttributeStringValue(ConfigurationList::iterator iter,
	                             const char *attribute,
	                             string &value);

	/**
	 * Get a string value from a list element identified by a attribute value
	 *
	 * @param  list      the list
	 * @param  id        the reference attribute
	 * @param  id_val    the reference attribute value
	 * @param  attribute the desired attribute
	 * @param  value     the desired value
	 * @return  true on success, false otherwise
	 */
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


// these functions should must be in .h file because they are templates

template <class T>
bool ConfigurationFile::getValue(ConfigurationList section, 
                                 const char *key, 
                                 T &val)
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

// TODO check if used as public fct
template <class T>
bool ConfigurationFile::getValue(ConfigurationList::iterator iter,
                                 T &val)
{
	const xmlpp::TextNode *nodeText;
	xmlpp::Node::NodeList list;
	xmlpp::Element *element ;
	string tmp_val;
	
	element = dynamic_cast<xmlpp::Element *>(*iter);
	list = element->get_children();
	if(list.size() != 1)
	{
		return false;
	}
	else
	{
		nodeText = dynamic_cast<const xmlpp::TextNode*>(list.front());
	}

	if(!nodeText)
	{
		return false;
	}
	tmp_val = nodeText->get_content();

	stringstream str(tmp_val);
	str >> val;
	if(str.fail())
	{
		return false;
	}

	return true;
}

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

template <class T>
bool ConfigurationFile::getElementWithAttributeValue(ConfigurationList list,
                                                     const char *attribute_name,
                                                     T &attribute_value,
                                                     ConfigurationList &elements)
{
	stringstream strs;
	string tmp_str;
	strs << attribute_value;
	tmp_str = strs.str();
	if(!this->getElementWithAttributeStringValue(list, attribute_name, 
		                                   tmp_str.c_str(), elements))
	{
		return false;
	}

	return true;
}

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


template <class T>
bool ConfigurationFile::getValueInList(ConfigurationList section,
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
inline bool ConfigurationFile::getValue<bool>(ConfigurationList section,
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
 * and we should not surcharge a specialization */
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
inline bool ConfigurationFile::getValue<uint8_t>(ConfigurationList section,
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

template <>
inline bool ConfigurationFile::getAttributeValue<stringstream>(ConfigurationList::iterator iter,
                                                               const char *attribute,
                                                               stringstream &value)
{
	string tmp_val;

	if(!this->getAttributeValue(iter, attribute, tmp_val))
		return false;

	value << tmp_val;

	return true;
}

template <>
inline bool ConfigurationFile::getElementWithAttributeValue<uint8_t>(
                                                ConfigurationList list,
                                                const char *attribute_name,
                                                uint8_t &attribute_value,
                                                ConfigurationList &elements)
{
	stringstream strs;
	string tmp_str;
	unsigned int val;
	val = attribute_value;
	strs << val;
	tmp_str = strs.str();
	if(!this->getElementWithAttributeStringValue(list, attribute_name, 
		                                   tmp_str.c_str(), elements))
	{
		return false;
	}

	return true;
}

/* only write this specialization because it will be called by the other one
 * and we should not surcharge a specialization */
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


template <>
inline bool ConfigurationFile::getAttributeValue<uint16_t>(ConfigurationList::iterator iter,
                                                           const char *attribute,
                                                           uint16_t &value)
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
inline bool ConfigurationFile::getValueInList<uint16_t>(ConfigurationList list,
                                                        const char *id,
                                                        const string id_val,
                                                        const char *attribute,
                                                        uint16_t &value)
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


#endif
