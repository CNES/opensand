/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 CNES
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
 * @file OpenSandPlugin.h
 * @brief Generic OpenSAND plugin
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef OPENSAND_PLUGIN_H
#define OPENSAND_PLUGIN_H

#include <string>
#include <cstdlib>
#include <cstring>

#define PLUGIN_MAX_LEN 256

typedef enum
{
	unknown,
	encapsulation_plugin,
	attenuation_plugin,
	nominal_plugin,
	minimal_plugin,
	error_plugin,
} plugin_type_t;

class OpenSandPlugin;

typedef OpenSandPlugin *(*fn_create)();

typedef struct
{
	fn_create create;
	plugin_type_t type;
	char name[PLUGIN_MAX_LEN];
} opensand_plugin_t;

typedef opensand_plugin_t *fn_init();

/**
 * @class OpenSandPlugin
 * @brief Generic OpenSAND plugin
 */
class OpenSandPlugin
{

 public:

	/**
	 * @brief Plugin constructor
	 */
	OpenSandPlugin() {};

	/**
	 * @brief Plugins destuctor
	 */
	virtual ~OpenSandPlugin() {};

	/**
	 * @brief Create the plugin, this function should be called instead of constructor
	 *
	 * @return the plugin
	 */
	template<class Plugin>
	static OpenSandPlugin *create(std::string name)
	{
		Plugin *plugin = new Plugin();
		plugin->name = name;
		return plugin;
	};

	/**
	 * @brief Initialize the plugin
	 *
	 * @return the plugin data
	 */
	static opensand_plugin_t *init;

  protected:

	std::string name;

};

/// Define the function that will create the plugin class
#define CREATE(CLASS, pl_type, pl_name) \
	extern "C" OpenSandPlugin *create_ptr(){return CLASS::create<CLASS>(pl_name);}; \
	extern "C" opensand_plugin_t *init() \
	{\
		opensand_plugin_t *pl = (opensand_plugin_t *)calloc(1, sizeof(opensand_plugin_t)); \
		pl->create = create_ptr; \
		pl->type = pl_type; \
		strncpy(pl->name, pl_name, PLUGIN_MAX_LEN - 1); \
		pl->name[PLUGIN_MAX_LEN - 1] = '\0'; \
		return pl; \
	};
#endif
