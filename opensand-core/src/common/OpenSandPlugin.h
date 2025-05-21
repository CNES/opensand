/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.com>
 */

#ifndef OPENSAND_PLUGIN_H
#define OPENSAND_PLUGIN_H


#include <string>


class OpenSandPlugin;
struct OpenSandPluginFactory;
typedef OpenSandPlugin *(*fn_create)(void);
typedef void (*fn_configure)(const char *, const char *);
typedef OpenSandPluginFactory *fn_init(void);


enum class PluginType
{
	Unknown,
	Encapsulation,
	Attenuation,
	Minimal,
	Error,
	SatDelay,
};


struct OpenSandPluginFactory
{
	fn_create create;
	fn_configure configure;
	PluginType type;
	std::string name;
};


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
	virtual ~OpenSandPlugin() = default;

	/**
	 * @brief Create the plugin, this function should be called instead of constructor
	 *
	 * @return the plugin
	 */
	template<class Plugin>
	static OpenSandPlugin *create(const std::string &name)
	{
		auto plugin = new Plugin();
		plugin->name = name;
		return plugin;
	};

	/**
	 * @brief Generate the configuration for the plugin
	 */
	template<class Plugin>
	static void configure(const std::string &parent_path,
	                      const std::string &param_id,
	                      const std::string &name)
	{
		Plugin::generateConfiguration(parent_path, param_id, name);
	};

	/**
	 * @brief Get the plugin name
	 *
	 * @return the plugin name
	 */
	inline std::string getName() const {return this->name;};

protected:
	std::string name;
};


/// Define the function that will create the plugin class
#define CREATE(CLASS, pl_type, pl_name) \
	extern "C" OpenSandPlugin *create_ptr(void){return CLASS::create<CLASS>(pl_name);}; \
	extern "C" void configure_ptr(const char *parent_path, const char *param_id) \
	{\
		CLASS::configure<CLASS>(parent_path, param_id, pl_name); \
	}; \
	extern "C" OpenSandPluginFactory *init(void) \
	{\
		auto pl = new OpenSandPluginFactory{ \
			create_ptr, \
			configure_ptr, \
			pl_type, \
			pl_name \
		}; \
		return pl; \
	};


#endif
