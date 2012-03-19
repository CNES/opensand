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
 * @file ConfigurationFile.cpp
 * @brief Reading parameters from a configuration file
 * @author Viveris Technologies
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "uti_debug.h" // use default package
#include "ConfigurationFile.h"


ConfigurationFile globalConfig;


/**
 * Ctor
 */
ConfigurationFile::ConfigurationFile()
{
	// uncomment to get debug messages during file loading
	// (because levels haven't yet been read from config file)
	//UTI_DEBUG_LEVEL(2);
}


/**
 * dtor = free association table
 */
ConfigurationFile::~ConfigurationFile()
{
	unloadConfig();
}



/**
 * Load the whole configuration file content into memory
 * @param confFile path and name of the configuration file
 * @return  true on success, false otherwise
 */
bool ConfigurationFile::loadConfig(const string confFile)
{
	string section, key, value;
	bool ret = false;
	xmlpp::DomParser *new_parser;
	const xmlpp::Element *root;

	// Build whole path and name of configuration file
	if(confFile.empty())
	{
		UTI_ERROR("Configuration filename is empty\n");
		goto FCT_END;
	}

	if(access(confFile.c_str(), R_OK) < 0)
	{
		UTI_ERROR("unable to access configuration file '%s' (%s)\n",
		          confFile.c_str(), strerror(errno));
		goto FCT_END;
	}

	try
	{
		new_parser = new xmlpp::DomParser();
		new_parser->set_substitute_entities();
		new_parser->parse_file(confFile);
		root = new_parser->get_document()->get_root_node();
		if(root->get_name() != "configuration")
		{
			UTI_ERROR("Root element is not 'configuration' (%s)\n",
			          root->get_name().c_str());
			goto FCT_END;
		}
		this->_parsers.push_back(new_parser);
	}
	catch(const std::exception& ex)
	{
		UTI_ERROR("Exception when parsing the configuration file %s: %s\n",
		          confFile.c_str(), ex.what());
		goto FCT_END;
	}

	ret = true;

FCT_END:
	return ret;
}


/**
 * Unload the whole configuration file content from memory,
 * i.e. free tables
 */
void ConfigurationFile::unloadConfig()
{
	vector<xmlpp::DomParser *>::iterator parser;

	for(parser = this->_parsers.begin(); parser != this->_parsers.end(); parser++)
	{
		delete *parser;
	}
	this->_parsers.clear();
} // unloadConfig

/**
 * Get a XML section node from its name
 *
 * @param  section      name of the section
 * @param  sectionNode  the XML section node
 * @return  true on success, false otherwise
 */
bool ConfigurationFile::getSection(const char *section,
                                   xmlpp::Node::NodeList &sectionList)
{
	vector<xmlpp::DomParser *>::iterator parser;

	for(parser = this->_parsers.begin(); parser != this->_parsers.end(); parser++)
	{
		const xmlpp::Element* root;
		xmlpp::Node::NodeList tempList;

		root = (*parser)->get_document()->get_root_node();
		tempList = root->get_children(section);
		sectionList.insert(sectionList.end(), tempList.begin(), tempList.end());
	}
	if(sectionList.empty())
	{
		UTI_ERROR("no section '%s'\n", section);
		goto error;
	}

	return true;
error:
	return false;
}

/**
 * Get a XML key node from its name and its section name
 *
 * @param  section  name of the section
 * @param  key      name of the key
 * @param  keyNode  the XML key node
 * @return  true on success, false otherwise
 */
bool ConfigurationFile::getKey(const char *section,
                               const char *key,
                               const xmlpp::Element **keyNode)
{
	xmlpp::Node::NodeList sectionList;
	xmlpp::Node::NodeList::iterator iter;
	xmlpp::Node::NodeList keyList;
	bool found = false;

	if(!this->getSection(section, sectionList))
	{
		UTI_ERROR("cannot find section %s\n", section);
		goto error;
	}

	for(iter = sectionList.begin(); iter != sectionList.end(); iter++)
	{
		const xmlpp::Node *sectionNode = *iter;

		keyList = sectionNode->get_children(key);
		if(keyList.size() > 1)
		{
			UTI_ERROR("more than one key named '%s' in section '%s'\n",
					  key, section);
			goto error;
		}
		else if(keyList.size() == 1)
		{
			*keyNode = dynamic_cast<const xmlpp::Element*>(keyList.front());
			if(!(*keyNode))
			{
				UTI_ERROR("cannot convert the key '%s' from section '%s' "
						  "into element\n", key, section);
				goto error;
			}
			found = true;
			break;
		}
	}
	if(!found)
	{
		UTI_ERROR("no key named '%s' in section '%s'\n",
				key, section);
		goto error;
	}

error:
	return found;
}


/**
 * Read a string value from configuration
 *
 * @param  section  name of the section
 * @param  key      name of the key
 * @param  value    value of the string
 * @return  true on success, false otherwise
 */
bool ConfigurationFile::getStringValue(const char *section,
                                       const char *key,
                                       string &value)
{
	const xmlpp::Element *keyNode;
	const xmlpp::TextNode *nodeText;
	xmlpp::Node::NodeList list;

	if(!this->getKey(section, key, &keyNode))
	{
		goto error;
	}

	list = keyNode->get_children();
	if(list.size() != 1)
	{
		UTI_ERROR("The key '%s' in section '%s' does not contain text\n",
		          key, section);
		goto error;
	}
	else
	{
		nodeText = dynamic_cast<const xmlpp::TextNode*>(list.front());
	}

	if(!nodeText)
	{
		UTI_ERROR("The key '%s' in section '%s' does not contain text\n",
		          key, section);
		goto error;
	}
	value = nodeText->get_content();

	return true;
error:
	return false;
}
	

/**
 * Read an integer value from configuration
 *
 * @param  section  name of the section
 * @param  key      name of the key
 * @param  value    integer value
 * @return  true on success, false otherwise
 */
bool ConfigurationFile::getIntegerValue(const char *section,
                                        const char *key,
                                        int &value)
{
	string valueStr;

	if(this->getStringValue(section, key, valueStr) &&
	   (valueStr.size() > 0))
	{
		value = atoi(valueStr.c_str());
		return true;
	}

	return false;
} // getIntegerValue


/**
 * Read a longeger value from configuration
 *
 * @param  section  name of the section
 * @param  key      name of the key
 * @param  value    integer value
 * @return  true on success, false otherwise
 */
bool ConfigurationFile::getLongIntegerValue(const char *section,
                                            const char *key,
                                            long &value)
{
	string valueStr;

	if(this->getStringValue(section, key, valueStr) &&
	   (valueStr.size() > 0))
	{
		value = atol(valueStr.c_str());
		return true;
	}

	return false;
}


/**
 * Read the number of elements in a list
 *
 * @param  section  name of the section
 * @param  key      name of the list key
 * @param  nbr      the number of elements in the list
 * @return  true on success, false otherwise
 */
bool ConfigurationFile::getNbListItems(const char *section,
                                       const char *key,
                                       int &nbr)
{
	ConfigurationList list;
	if(!this->getListItems(section, key, list))
	{
		goto error;
	}

	nbr = list.size();

	return true;
error:
	return false;

} // getNbListItems


/**
 * Get the elements from the list
 *
 * @param  section  name of the section
 * @param  key      name of the list key
 * @param  list     the list
 * @return  true on success, false otherwise
 */
bool ConfigurationFile::getListItems(const char *section,
                                     const char *key,
                                     ConfigurationList &list)
{
	const xmlpp::Element *keyNode;
	xmlpp::Node::NodeList tempList;
	xmlpp::Node::NodeList::iterator iter;

	if(!this->getKey(section, key, &keyNode))
	{
		goto error;
	}

	tempList = keyNode->get_children();
	for(iter = tempList.begin(); iter != tempList.end(); iter++)
	{
		const xmlpp::TextNode* nodeText;
		const xmlpp::CommentNode* nodeComment;
		Glib::ustring nodename;

		nodeText = dynamic_cast<const xmlpp::TextNode*>(*iter);
		nodeComment = dynamic_cast<const xmlpp::CommentNode*>(*iter);
		nodename = (*iter)->get_name();

		if(!nodeText && !nodeComment && !nodename.empty()) //Let's not say "name: text".
		{   
			list.push_back(*iter);
		}   
	}

	return true;
error:
	return false;
}

/**
 * Get the string value of an attribute in a list element
 *
 * @param  elt        an iterator on a ConfigurationList
 * @param  attribute  the attribute name
 * @param  value      attribute value
 * @return  true on success, false otherwise
 */
bool ConfigurationFile::getAttributeStringValue(ConfigurationList::iterator iter,
                                               const char *attribute,
                                               string &value)
{
	const xmlpp::Attribute *name;
	const xmlpp::Element *element;

	element = dynamic_cast<const xmlpp::Element *>(*iter);
	if(!element)
	{
		UTI_ERROR("Wrong configuration list element\n");
		goto error;
	}
	name = element->get_attribute(attribute);
	if(!name)
	{
		UTI_ERROR("no attribute named %s in element %s\n",
		          attribute, element->get_name().c_str());
		goto error;
	}
	else
	{
		value = name->get_value();
	}

	return true;
error:
	return false;
}


/**
 * Get the integer value of an attribute in a list element
 *
 * @param  elt        an iterator on a ConfigurationList
 * @param  attribute  the attribute name
 * @param  value      attribute value
 * @return  true on success, false otherwise
 */
bool ConfigurationFile::getAttributeIntegerValue(ConfigurationList::iterator iter,
                                                 const char *attribute,
                                                 int &value)
{
	string valueStr;

	if(this->getAttributeStringValue(iter, attribute, valueStr) &&
	   (valueStr.size() > 0))
	{
		value = atoi(valueStr.c_str());
		return true;
	}

	return false;
}

/**
 * Get the longeger value of an attribute in a list element
 *
 * @param  elt        an iterator on a ConfigurationList
 * @param  attribute  the attribute name
 * @param  value      attribute value
 * @return  true on success, false otherwise
 */
bool ConfigurationFile::getAttributeLongIntegerValue(ConfigurationList::iterator iter,
                                                     const char *attribute,
                                                     long &value)

{
	string valueStr;

	if(this->getAttributeStringValue(iter, attribute, valueStr) &&
	   (valueStr.size() > 0))
	{
		value = atol(valueStr.c_str());
		return true;
	}

	return false;
}

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
bool ConfigurationFile::getStringValueInList(ConfigurationList list,
                                             const char *id,
                                             const string id_val,
                                             const char *attribute,
                                             string &value)
{
	ConfigurationList::iterator iter;

	for(iter = list.begin(); iter != list.end(); iter++)
	{
		string ref;

		if(!this->getAttributeStringValue(iter, id, ref))
		{
			goto error;
		}	
		if(ref == id_val)
		{
			continue;
		}
		// we are on the desired line
		return this->getAttributeStringValue(iter, attribute, value);
	}

error:
	return false;
}

/**
 * Get a string value from a list element identified by a attribute value
 *
 * @param  section   name of the section identifying the list
 * @param  key       name of the list key identifying the list
 * @param  id        the reference attribute
 * @param  id_val    the reference attribute value
 * @param  attribute the desired attribute
 * @param  value     the desired value
 * @return  true on success, false otherwise
 */
bool ConfigurationFile::getStringValueInList(const char *section,
                                             const char *key,
                                             const char *id,
                                             const string id_val,
                                             const char *attribute,
                                             string &value)
{
	ConfigurationList list;

	if(!this->getListItems(section, key, list))
	{
		goto error;
	}
	return this->getStringValueInList(list, id, id_val, attribute, value);

error:
	return false;
}


/**
 * Get an integer value from a list element identified by a attribute value
 *
 * @param  list      the list
 * @param  id        the reference attribute
 * @param  id_val    the reference attribute value
 * @param  attribute the desired attribute
 * @param  value     the desired value
 * @return  true on success, false otherwise
 */
bool ConfigurationFile::getIntegerValueInList(ConfigurationList list,
                                              const char *id,
                                              const string id_val,
                                              const char *attribute,
                                              int &value)
{
	string valueStr;

	if(this->getStringValueInList(list, id, id_val, attribute, valueStr)
	   && (valueStr.size() > 0))
	{
		value = atoi(valueStr.c_str());
		return true;
	}

	return false;
}

/**
 * Get an integer value from a list element identified by a attribute value
 *
 * @param  section   name of the section identifying the list
 * @param  key       name of the list key identifying the list
 * @param  id        the reference attribute
 * @param  id_val    the reference attribute value
 * @param  attribute the desired attribute
 * @param  value     the desired value
 * @return  true on success, false otherwise
 */
bool ConfigurationFile::getIntegerValueInList(const char *section,
                                              const char *key,
                                              const char *id,
                                              const string id_val,
                                              const char *attribute,
                                              int &value)
{
	string valueStr;

	if(this->getStringValueInList(section, key, id, id_val, attribute, valueStr)
	   && (valueStr.size() > 0))
	{
		value = atoi(valueStr.c_str());
		return true;
	}

	return false;
}

/**
 * Get a longeger value from a list element identified by a attribute value
 *
 * @param  list      the list
 * @param  id        the reference attribute
 * @param  id_val    the reference attribute value
 * @param  attribute the desired attribute
 * @param  value     the desired value
 * @return  true on success, false otherwise
 */
bool ConfigurationFile::getLongIntegerValueInList(ConfigurationList list,
                                                  const char *id,
                                                  const string id_val,
                                                  const char *attribute,
                                                  long &value)
{
	string valueStr;

	if(this->getStringValueInList(list, id, id_val, attribute, valueStr)
	   && (valueStr.size() > 0))
	{
		value = atol(valueStr.c_str());
		return true;
	}

	return false;
}

/**
 * Get a longeger value from a list element identified by a attribute value
 *
 * @param  section   name of the section identifying the list
 * @param  key       name of the list key identifying the list
 * @param  id        the reference attribute
 * @param  id_val    the reference attribute value
 * @param  attribute the desired attribute
 * @param  value     the desired value
 * @return  true on success, false otherwise
 */
bool ConfigurationFile::getLongIntegerValueInList(const char *section,
                                                  const char *key,
                                                  const char *id,
                                                  const string id_val,
                                                  const char *attribute,
                                                  long &value)
{
	string valueStr;

	if(this->getStringValueInList(section, key, id, id_val, attribute, valueStr)
	   && (valueStr.size() > 0))
	{
		value = atol(valueStr.c_str());
		return true;
	}

	return false;
}


