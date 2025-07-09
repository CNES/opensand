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
 * @file PluginUtils.cpp
 * @brief utilities for Plugins
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "PluginUtils.h"
#include "EncapPlugin.h"
#include "LanAdaptationPlugin.h"
#include "PhysicalLayerPlugin.h"
#include "IslPlugin.h"
#include "OpenSandCore.h"
#include "OpenSandModelConf.h"

#include <dirent.h>
#include <errno.h>
#include <dlfcn.h>

#include <opensand_output/Output.h>


extern const std::string PLUGIN_LIBDIR;
const std::string PLUGIN_DIRECTORY{"/opensand/plugins/"};
const std::string PLUGIN_FILE_END = ".so.0";


PluginUtils::PluginUtils()
{
}


bool PluginUtils::loadPlugins(bool enable_phy_layer)
{
	std::vector<std::string> path;
	this->log_init = Output::Get()->registerLog(LEVEL_WARNING, "init");

	char *lib_path = getenv("LD_LIBRARY_PATH");
	if(lib_path)
	{
		// Split using ':' separator
		tokenize(lib_path, path, ":");
	}
	path.push_back(PLUGIN_LIBDIR);

	for(auto& directory : path)
	{
		std::string dir = directory + PLUGIN_DIRECTORY;
		

		DIR *plugin_dir = opendir(dir.c_str());
		if(!plugin_dir)
		{
			LOG(this->log_init, LEVEL_NOTICE,
			    "cannot search plugins in %s folder\n", 
			    dir.c_str());
			continue;
		}
		LOG(this->log_init, LEVEL_NOTICE,
		    "search for plugins in %s folder\n", dir.c_str());

		struct dirent *ent;
		while((ent = readdir(plugin_dir)) != NULL)
		{
			std::string filename = ent->d_name;
			if(filename.length() <= PLUGIN_FILE_END.length())
			{
				continue;
			}
			if(!filename.compare(filename.length() - PLUGIN_FILE_END.length(),
			                     PLUGIN_FILE_END.length(),
								 PLUGIN_FILE_END))
			{
				std::string plugin_name = dir + filename;

				LOG(this->log_init, LEVEL_INFO,
				    "find plugin library %s\n", filename.c_str());
				void *handle = dlopen(plugin_name.c_str(), RTLD_LAZY);
				if(!handle)
				{
					LOG(this->log_init, LEVEL_ERROR,
					    "cannot load plugin %s (%s)\n",
					    filename.c_str(), dlerror());
					continue;
				}

				void *sym = dlsym(handle, "init");
				if(!sym)
				{

					LOG(this->log_init, LEVEL_ERROR,
					    "cannot find 'init' method in plugin %s "
					    "(%s)\n", filename.c_str(), dlerror());
					dlclose(handle);
					closedir(plugin_dir);
					return false;
				}

				OpenSandPluginFactory *plugin = reinterpret_cast<fn_init *>(sym)();
				if(!plugin)
				{
					LOG(this->log_init, LEVEL_ERROR,
					    "cannot create plugin\n");
					dlclose(handle);
					continue;
				}

				switch(plugin->type)
				{
					case PluginType::Encapsulation:
						storePlugin(this->encapsulation, plugin, handle);
						break;

					case PluginType::IslDelay:
						storePlugin(this->isl_delay, plugin, handle);
						break;

					case PluginType::SatDelay:
						storePlugin(this->sat_delay, plugin, handle);
						break;

					case PluginType::Attenuation:
						if(!enable_phy_layer)
						{
							dlclose(handle);
						}
						else
						{
							storePlugin(this->attenuation, plugin, handle);
						}
						break;

					case PluginType::Minimal:
						if(!enable_phy_layer)
						{
							dlclose(handle);
						}
						else
						{
							storePlugin(this->minimal, plugin, handle);
						}
						break;

					case PluginType::Error:
						if(!enable_phy_layer)
						{
							dlclose(handle);
						}
						else
						{
							storePlugin(this->error, plugin, handle);
						}
						break;

					default:
						LOG(this->log_init, LEVEL_ERROR,
						    "Wrong plugin type %d for %s",
						    plugin->type, filename.c_str());
				}
				delete plugin;
			}
		}
		closedir(plugin_dir);
	}

	return true;
}


void PluginUtils::storePlugin(PluginConfigurationContainer &container, OpenSandPluginFactory *plugin, void *handle)
{
	const std::string plugin_name = plugin->name;

	// if we load twice the same plugin, keep the first one
	// this is why LD_LIBRARY_PATH should be first in the paths
	if(container.find(plugin_name) == container.end())
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "load plugin %s\n",
		    plugin_name.c_str());
		container[plugin_name] = {
			plugin->configure,
			plugin->create,
			nullptr,
		};
		this->handlers.push_back(handle);
	}
	else
	{
		dlclose(handle);
	}
}


inline void releasePluginsContainer(PluginConfigurationContainer &container)
{
	for (auto &&[name, element] : container)
	{
		delete element.plugin;
	}
}


void PluginUtils::releasePlugins()
{
	releasePluginsContainer(encapsulation);
	releasePluginsContainer(lan_adaptation);
	releasePluginsContainer(attenuation);
	releasePluginsContainer(minimal);
	releasePluginsContainer(error);
	releasePluginsContainer(sat_delay);

	for(auto &&handler : this->handlers)
	{
		dlclose(handler);
	}
}


/**
 * @brief helper function to factorize getting any kind of plugin
 *
 * @param plugin_name  The name of the plugin to retrieve
 * @param container    The container where to look for the plugin
 * @param plugin       The plugin
 * @return true on success, false otherwise
 */
template<class PluginType>
bool getPlugin(const std::string &plugin_name,
               PluginConfigurationContainer &container,
               PluginType **plugin)
{
	auto plugin_configuration = container.find(plugin_name);
	if (plugin_configuration == container.end())
	{
		return false;
	}

	PluginConfigurationElement &configuration = plugin_configuration->second;
	if (configuration.plugin != nullptr)
	{
		//to manage virtual inheritance (used to avoid DDoD)
		assert(dynamic_cast<PluginType *>(configuration.plugin) != nullptr);
		*plugin = dynamic_cast<PluginType *>(configuration.plugin);
		return true;
	}

	if(!configuration.create)
	{
		return false;
	}

	*plugin = dynamic_cast<PluginType *>(configuration.create());
	if(!*plugin)
	{
		return false;
	}

	configuration.plugin = *plugin;
	return true;
}


bool PluginUtils::getEncapsulationPlugin(std::string name,
                                         EncapPlugin **encapsulation)
{
	return getPlugin(name, this->encapsulation, encapsulation);
};


bool PluginUtils::getIslDelayPlugin(std::string name,
                                    IslDelayPlugin **sat_delay)
{
	return getPlugin(name, this->isl_delay, sat_delay);
};


bool PluginUtils::getSatDelayPlugin(std::string name,
                                    SatDelayPlugin **sat_delay)
{
	return getPlugin(name, this->sat_delay, sat_delay);
};


bool PluginUtils::getAttenuationPlugin(std::string name,
                                       AttenuationModelPlugin **attenuation)
{
	if(name.size() > 0)
	{
		return getPlugin(name, this->attenuation, attenuation);
	}

	return true;
};


bool PluginUtils::getMinimalConditionPlugin(std::string name,
                                            MinimalConditionPlugin **minimal)
{
	if(name.size() > 0)
	{
		return getPlugin(name, this->minimal, minimal);
	}

	return true;
};


bool PluginUtils::getErrorInsertionPlugin(std::string name,
                                          ErrorInsertionPlugin **error)
{
	if(name.size() > 0)
	{
		return getPlugin(name, this->error, error);
	}

	return true;
};


void PluginUtils::generatePluginsConfiguration(std::shared_ptr<OpenSANDConf::MetaComponent> parent,
                                               PluginType plugin_type,
                                               const std::string &parameter_id,
                                               const std::string &parameter_name,
                                               const std::string &parameter_description)
{
	PluginConfigurationContainer *container;

	switch(plugin_type)
	{
		case PluginType::Encapsulation:
			container = &this->encapsulation;
			break;

		case PluginType::IslDelay:
			container = &this->isl_delay;
			break;

		case PluginType::SatDelay:
			container = &this->sat_delay;
			break;

		case PluginType::Attenuation:
			container = &this->attenuation;
			break;

		case PluginType::Minimal:
			container = &this->minimal;
			break;

		case PluginType::Error:
			container = &this->error;
			break;

		default:
			LOG(this->log_init, LEVEL_ERROR,
				"Unable to generate configuration for plugin type %d",
				plugin_type);
			return;
	}

	const std::string type_name = std::string{"plugin_"} + parameter_id;
	std::vector<std::string> plugin_names;
	plugin_names.reserve(container->size());
	for(auto const &element : *container)
	{
		plugin_names.push_back(element.first);
	}

	auto Conf = OpenSandModelConf::Get();
	auto types = Conf->getModelTypesDefinition();
	types->addEnumType(type_name, parameter_name, plugin_names);

	std::string parent_path;
	if (parent != nullptr)
	{
		parent_path = parent->getPath();
		parent->addParameter(parameter_id, parameter_name, types->getType(type_name), parameter_description);
	}

	const char *path = parent_path.c_str();
	const char *param_id = parameter_id.c_str();
	for(auto const &element : *container)
	{
		fn_configure configure = element.second.init;
		if(configure)
		{
			configure(path, param_id);
		}
	}
}

std::vector<std::string> PluginUtils::getAllEncapsulationPlugins()
{
	std::vector<std::string> names;
	names.reserve(encapsulation.size());
	for (auto &&[name, element] : encapsulation)
	{
		names.push_back(name);
	}
	return names;
}
