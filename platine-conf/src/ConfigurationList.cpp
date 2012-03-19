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
 * @file ConfigurationList.cpp
 * @brief The representation of a list from XML configuration file
 * @author Viveris Technologies
 */

#include "ConfigurationList.h"

ConfigurationList::ConfigurationList(): xmlpp::Node::NodeList()
{
	xmlpp::Element::AttributeList attributes;
	xmlpp::Element::AttributeList::const_iterator iter;
	const xmlpp::Element *elem;

	if(!this->size())
	{
		return;
	}

	elem = dynamic_cast<const xmlpp::Element *>(this->front());
	if(elem)
	{
		attributes = elem->get_attributes();
	    for(iter = attributes.begin(); iter != attributes.end(); iter++)
	    {
			this->_attributes.push_back((*iter)->get_name());
		}
	}
}

ConfigurationList::~ConfigurationList()
{
	this->_attributes.clear();
}

/** Get the attributes from the list
 *
 *  @param attributes The attributes
 *  @return true on success, false otherwise
 */
bool ConfigurationList::getAttributes(vector<string> &attributes)
{
	if(this->_attributes.size() > 0)
	{
		attributes = this->_attributes;
		return true;
	}

	return false;
}

