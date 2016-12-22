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

#include "ConfigurationFile.h"
#include "conf.h"

#include <opensand_output/Output.h>



static map<string, log_level_t> createLevelsMap()
{
	map<string, log_level_t> levels;
	levels["debug"]     = LEVEL_DEBUG;
	levels["info"]      = LEVEL_INFO;
	levels["notice"]    = LEVEL_NOTICE;
	levels["warning"]   = LEVEL_WARNING;
	levels["error"]     = LEVEL_ERROR;
	levels["critical"]  = LEVEL_CRITICAL;
	return levels;
}

static map<string, log_level_t> levels_map = createLevelsMap();

static log_level_t logNameToLevel(string name)
{
	return levels_map[name];
}


/**
 * Ctor
 */
ConfigurationFile::ConfigurationFile()
{
}


/**
 * dtor = free association table
 */
ConfigurationFile::~ConfigurationFile()
{
	this->unloadConfig();
}

bool ConfigurationFile::loadConfig(const string conf_file)
{
	vector<string> file(1, conf_file);
	return this->loadConfig(file);
}

bool ConfigurationFile::loadConfig(const vector<string> conf_files)
{
	string section, key, value;
	xmlpp::DomParser *new_parser;
	const xmlpp::Element *root;
	vector<string>::const_iterator it;

	// Output Log
	this->log_conf = Output::registerLog(LEVEL_WARNING, "Conf");

	if(conf_files.size() == 0)
	{
		LOG(this->log_conf, LEVEL_ERROR,
		    "No configuration files provided\n");
		return false;
	}

	for(it = conf_files.begin(); it != conf_files.end(); ++it)
	{
		if((*it).empty())
		{
			LOG(this->log_conf, LEVEL_ERROR,
			    "Configuration filename is empty\n");
			return false;
		}

		if(access((*it).c_str(), R_OK) < 0)
		{
			LOG(this->log_conf, LEVEL_ERROR,
			    "unable to access configuration file '%s' (%s)\n",
			    (*it).c_str(), strerror(errno));
			return false;
		}

		try
		{
			new_parser = new xmlpp::DomParser();
			new_parser->set_substitute_entities();
			new_parser->parse_file((*it));
			root = new_parser->get_document()->get_root_node();
			if(root->get_name() != "configuration")
			{
				LOG(this->log_conf, LEVEL_ERROR,
				    "Root element is not 'configuration' (%s)\n",
				    root->get_name().c_str());
				return false;
			}
			this->parsers.push_back(new_parser);
		}
		catch(const std::exception& ex)
		{
			LOG(this->log_conf, LEVEL_ERROR,
			    "Exception when parsing the configuration file %s: %s\n",
			    (*it).c_str(), ex.what());
			return false;
		}
	}

	return true;
}


void ConfigurationFile::unloadConfig()
{
	vector<xmlpp::DomParser *>::iterator parser;

	for(parser = this->parsers.begin(); parser != this->parsers.end(); parser++)
	{
		delete *parser;
	}
	this->parsers.clear();
}


bool ConfigurationFile::getComponent(string &compo)
{
	vector<xmlpp::DomParser *>::iterator parser;

	for(parser = this->parsers.begin(); parser != this->parsers.end(); parser++)
	{
		const xmlpp::Attribute *name;
		const xmlpp::Element *root;

		root = (*parser)->get_document()->get_root_node();
		name = root->get_attribute("component");
		if(!name)
		{
			LOG(this->log_conf, LEVEL_ERROR,
			    "Unable to find component attribute\n");
			continue;
		}
		else
		{
			string val = name->get_value();
			// we may be in the global config or topology or ...
			if(val == "st" || val == "gw" || val == "sat" || val == "ws")
			{
				compo = val;
				// we found the component
				return true;
			}
		}
	}
	return false;
}


bool ConfigurationFile::getSection(const char *section,
                                   xmlpp::Node::NodeList &sectionList)
{
	vector<xmlpp::DomParser *>::iterator parser;

	for(parser = this->parsers.begin(); parser != this->parsers.end(); parser++)
	{
		const xmlpp::Element* root;
		xmlpp::Node::NodeList tempList;

		root = (*parser)->get_document()->get_root_node();
		tempList = root->get_children(section);
		sectionList.insert(sectionList.end(), tempList.begin(), tempList.end());
	}
	if(sectionList.empty())
	{
		LOG(this->log_conf, LEVEL_ERROR,
		    "no section '%s'\n", section);
		goto error;
	}

	return true;
error:
	return false;
}

void ConfigurationFile::loadSectionMap(map<string, ConfigurationList> &section_map)
{
	vector<xmlpp::DomParser *>::iterator parser;
	ConfigurationList childrenList;

	for(parser = this->parsers.begin(); parser != this->parsers.end(); parser++)
	{
		const xmlpp::Element* root;
		xmlpp::Node::NodeList tempList;

		root = (*parser)->get_document()->get_root_node();
		tempList = root->get_children();
		childrenList.insert(childrenList.end(), tempList.begin(), tempList.end());
	}

	if(!childrenList.empty())
	{
		xmlpp::Node::NodeList::iterator iter;

		for(iter = childrenList.begin(); iter != childrenList.end(); iter++)
		{
			string name = ((xmlpp::Node*)*iter)->get_name();
			ConfigurationList sectionList;
			// if name is not already a key
			if(section_map.find(name.c_str()) == section_map.end())
			{
				this->getSection(name.c_str(), sectionList);
				section_map[name] = sectionList;
			}
		}
	}
}

bool ConfigurationFile::getKey(ConfigurationList sectionList,
                               const char *key,
                               const xmlpp::Element **keyNode)
{
	xmlpp::Node::NodeList::iterator iter;
	xmlpp::Node::NodeList keyList;
	bool found = false;

	
	if(sectionList.empty())
	{
		LOG(this->log_conf, LEVEL_ERROR, "section empty\n");
		goto error;
	}

	for(iter = sectionList.begin(); iter != sectionList.end(); iter++)
	{
		const xmlpp::Node *sectionNode = *iter;

		keyList = sectionNode->get_children(key);
		if(keyList.size() > 1)
		{
			LOG(this->log_conf, LEVEL_ERROR,
			    "more than one key named '%s' in section '%s'\n",
			    key, sectionNode->get_name().c_str());
			goto error;
		}
		else if(keyList.size() == 1)
		{
			*keyNode = dynamic_cast<const xmlpp::Element*>(keyList.front());
			if(!(*keyNode))
			{
				LOG(this->log_conf, LEVEL_ERROR,
				    "cannot convert the key '%s' from section '%s' "
				    "into element\n", key, sectionNode->get_name().c_str());
				goto error;
			}
			found = true;
			break;
		}
	}
	if(!found)
	{
		LOG(this->log_conf, LEVEL_ERROR,
		    "no key named '%s' in section '%s'\n",
		    key, ((xmlpp::Node*)(*sectionList.begin()))->get_name().c_str());
		goto error;
	}

error:
	return found;
}


bool ConfigurationFile::getStringValue(ConfigurationList sectionList,
                                       const char *key,
                                       string &value)
{
	const xmlpp::Element *keyNode;
	const xmlpp::TextNode *nodeText;
	xmlpp::Node::NodeList list;

	if(!this->getKey(sectionList, key, &keyNode))
	{
		goto error;
	}

	list = keyNode->get_children();
	if(list.size() != 1)
	{
		LOG(this->log_conf, LEVEL_ERROR,
		    "The key '%s' in section '%s' does not contain text\n",
		    key, ((xmlpp::Node*)(*sectionList.begin()))->get_name().c_str());
		goto error;
	}
	else
	{
		nodeText = dynamic_cast<const xmlpp::TextNode*>(list.front());
	}

	if(!nodeText)
	{
		LOG(this->log_conf, LEVEL_ERROR,
		    "The key '%s' in section '%s' does not contain text\n",
		    key, ((xmlpp::Node*)(*sectionList.begin()))->get_name().c_str());
		goto error;
	}
	value = nodeText->get_content();

	return true;
error:
	return false;
}

bool ConfigurationFile::getListNode(ConfigurationList sectionList,
                                    const char *key,
                                    xmlpp::Node::NodeList &nodeList)
{
	xmlpp::Node::NodeList::iterator iter;
	xmlpp::Node *sectionNode = NULL;
	bool found = false;

	for(iter = sectionList.begin(); iter != sectionList.end(); iter++)
	{
		sectionNode = *iter;
		xmlpp::Node::NodeList tempList = sectionNode->get_children(key);
		nodeList.insert(nodeList.end(), tempList.begin(), tempList.end());
		if(nodeList.size() == 0)
		{
			continue;
		}
		else
		{
			found = true;
		}
	}

	if(sectionList.empty())
	{
		LOG(this->log_conf, LEVEL_ERROR,
		    "section list is empty");
	}
	else if(!found)
	{
		LOG(this->log_conf, LEVEL_ERROR,
		    "there is no '%s' in section '%s'\n",
		    key, sectionNode->get_name().c_str());
	}
	return found;
}

bool ConfigurationFile::getElementWithAttributeStringValue(ConfigurationList list,
                                                     const char *attribute_name,
                                                     const char *attribute_value,
                                                     ConfigurationList &elements)
{
	ConfigurationList::iterator iter;
	for(iter = list.begin(); iter!=list.end(); iter++)
	{
		string id;

		if(!this->getAttributeStringValue(iter, attribute_name, id))
		{
			LOG(this->log_conf, LEVEL_ERROR,
			    "there is no attribute %s into %s",
			    attribute_name,
			    ((xmlpp::Node*)*iter)->get_name().c_str());
			return false;
		}

		if(strcmp(id.c_str(), attribute_value) == 0)
		{
			elements.push_back((xmlpp::Node*)*iter);
		}
	}

	if(elements.empty())
	{
		return false;
	}
	else
	{
		return true;
	}

}

bool ConfigurationFile::getNbListItems(ConfigurationList section,
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

}

bool ConfigurationFile::getListItems(xmlpp::Node *node,
                                     const char *key,
                                     ConfigurationList &list)
{
	ConfigurationList list_node;
	list_node.push_front(node);
	if(!this->getListItems(list_node, key, list))
	{
		return false;
	}

	return true;
}

bool ConfigurationFile::getListItems(ConfigurationList section,
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

		if(!nodeText && !nodeComment && !nodename.empty())
		{
			list.push_back(*iter);
		}
	}

	return true;
error:
	return false;
}

bool ConfigurationFile::getAttributeStringValue(ConfigurationList::iterator iter,
                                               const char *attribute,
                                               string &value)
{
	const xmlpp::Attribute *name;
	const xmlpp::Element *element;

	element = dynamic_cast<const xmlpp::Element *>(*iter);
	if(!element)
	{

		LOG(this->log_conf, LEVEL_ERROR,
		    "Wrong configuration list element\n");
		goto error;
	}
	name = element->get_attribute(attribute);
	if(!name)
	{
		LOG(this->log_conf, LEVEL_ERROR,
		    "no attribute named %s in element %s\n",
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
		if(ref != id_val)
		{
			continue;
		}
		// we are on the desired line
		return this->getAttributeStringValue(iter, attribute, value);
	}

error:
	return false;
}

bool ConfigurationFile::loadLevels(map<string, log_level_t> &levels,
                                   map<string, log_level_t> &specific)
{
	ConfigurationList sectionList;
	ConfigurationList::iterator sec_iter;
	ConfigurationList level_list;
	ConfigurationList::iterator iter;

	if(!this->getSection(SECTION_DEBUG, sectionList))
	{
		return false;
	}

	// load main levels
	for(sec_iter = sectionList.begin(); sec_iter != sectionList.end(); ++sec_iter)
	{
		const xmlpp::Node *sectionNode = *sec_iter;
		xmlpp::Node::NodeList::iterator key_iter;
		xmlpp::Node::NodeList keyList;

		keyList = sectionNode->get_children();
		for(key_iter = keyList.begin(); key_iter != keyList.end(); ++key_iter)
		{
			const xmlpp::TextNode* nodeText;
			const xmlpp::CommentNode* nodeComment;
			string key_name = (*key_iter)->get_name();
			string log_name = key_name;
			nodeText = dynamic_cast<const xmlpp::TextNode*>(*key_iter);
			nodeComment = dynamic_cast<const xmlpp::CommentNode*>(*key_iter);
			string val;
			if(nodeText || nodeComment)
			{
				continue;
			}
			if(key_name == "levels")
			{
				// ignore user defined levels list
				continue;
			}
			if(!this->getValue(sectionList, key_name.c_str(), val))
			{
				return false;
			}
			std::transform(key_name.begin(), key_name.end(),
			               log_name.begin(), ::tolower);
			levels[log_name] = logNameToLevel(val);
		}
	}

	// load specific levels
	if(!Conf::getListItems(sectionList, LEVEL_LIST,
                           level_list))
	{
		LOG(this->log_conf, LEVEL_ERROR,
		    "section '%s, %s': problem retrieving specific levels\n",
		    SECTION_DEBUG, LEVEL_LIST);
		return false;
	}

	for(iter = level_list.begin(); iter != level_list.end(); ++iter)
	{
		string log;
		string log_name;
		string level;

		// Get the Log Name
		if(!Conf::getAttributeValue(iter, LOG_NAME, log))
		{
			LOG(this->log_conf, LEVEL_ERROR,
			    "problem retrieving entry in levels\n");
			continue;
		}

		// Get the level
		if(!Conf::getAttributeValue(iter, LOG_LEVEL, level))
		{
			LOG(this->log_conf, LEVEL_ERROR,
			    "problem retrieving entry in levels\n");
			continue;
		}
		log_name = log;

		std::transform(log.begin(), log.end(),
		               log_name.begin(), ::tolower);
		specific[log_name] = logNameToLevel(level);
	}

	return true;
}


