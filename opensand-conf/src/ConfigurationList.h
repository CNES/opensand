/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file ConfigurationList.h
 * @brief The configuration list
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 */

#ifndef CONFIGURATION_LIST_H
#define CONFIGURATION_LIST_H

// silence warnings from the following include so we can hopefully use -Werror one day
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <libxml++/libxml++.h>
#pragma GCC diagnostic pop

using namespace std;

/*
 * @class ConfigurationList
 * @brief A list of items in configuration
 */
class ConfigurationList: public xmlpp::Node::NodeList
{
 public:
	 ConfigurationList(void);
	 ~ConfigurationList(void);

	 bool getAttributes(vector<string> &attributes);

 protected:
	 vector<string> attributes;
};

#endif

