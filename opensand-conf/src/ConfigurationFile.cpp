/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
			    key, section);
			goto error;
		}
		else if(keyList.size() == 1)
		{
			*keyNode = dynamic_cast<const xmlpp::Element*>(keyList.front());
			if(!(*keyNode))
			{
				LOG(this->log_conf, LEVEL_ERROR,
				    "cannot convert the key '%s' from section '%s' "
				    "into element\n", key, section);
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
		    key, section);
		goto error;
	}

error:
	return found;
}


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
		LOG(this->log_conf, LEVEL_ERROR,
		    "The key '%s' in section '%s' does not contain text\n",
		    key, section);
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
		    key, section);
		goto error;
	}
	value = nodeText->get_content();

	return true;
error:
	return false;
}

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

}

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


bool ConfigurationFile::loadLevels(map<string, log_level_t> &levels)
{
	xmlpp::Node::NodeList sectionList;
		xmlpp::Node::NodeList::iterator sec_iter;

	if(!this->getSection(SECTION_DEBUG, sectionList))
	{
		return false;
	}

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
			nodeText = dynamic_cast<const xmlpp::TextNode*>(*key_iter);
			nodeComment = dynamic_cast<const xmlpp::CommentNode*>(*key_iter);
			int val;
			if(nodeText || nodeComment)
			{
				continue;
			}
			if(!this->getValue(SECTION_DEBUG, key_name.c_str(), val))
			{
				return false;
			}
			levels[key_name] = (log_level_t)val;
		}
	}

	return true;

}


