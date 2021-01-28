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
#include "OpenSandCore.h"
#include "OpenSandModelConf.h"

#include <dirent.h>
#include <errno.h>
#include <dlfcn.h>

#include <opensand_output/Output.h>


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
	path.push_back("/usr/lib/");
	path.push_back("/lib");

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

				opensand_plugin_t *plugin = reinterpret_cast<fn_init *>(sym)();
				if(!plugin)
				{
					LOG(this->log_init, LEVEL_ERROR,
					    "cannot create plugin\n");
					dlclose(handle);
					continue;
				}

				switch(plugin->type)
				{
					case encapsulation_plugin:
						storePlugin(this->encapsulation, plugin, handle);
						break;

					case satdelay_plugin:
						storePlugin(this->sat_delay, plugin, handle);
						break;

					case lan_adaptation_plugin:
						storePlugin(this->lan_adaptation, plugin, handle);
						break;

					case attenuation_plugin:
						if(!enable_phy_layer)
						{
							dlclose(handle);
						}
						else
						{
							storePlugin(this->attenuation, plugin, handle);
						}
						break;

					case minimal_plugin:
						if(!enable_phy_layer)
						{
							dlclose(handle);
						}
						else
						{
							storePlugin(this->minimal, plugin, handle);
						}
						break;

					case error_plugin:
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


void PluginUtils::storePlugin(pl_list_t &container, opensand_plugin_t *plugin, void *handle)
{
	const std::string plugin_name = plugin->name;

	// if we load twice the same plugin, keep the first one
	// this is why LD_LIBRARY_PATH should be first in the paths
	if(container.find(plugin_name) == container.end())
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "load plugin %s\n",
		    plugin_name.c_str());
		container[plugin_name] = std::make_pair(plugin->configure, plugin->create);
		this->handlers.push_back(handle);
	}
	else
	{
		dlclose(handle);
	}
}


void PluginUtils::releasePlugins()
{
	for(std::vector<OpenSandPlugin *>::iterator iter = this->plugins.begin();
	    iter != this->plugins.end(); ++iter)
	{
		delete (*iter);
	}

	for(std::vector<void *>::iterator iter = this->handlers.begin();
	    iter != this->handlers.end(); ++iter)
	{
		dlclose(*iter);
	}
}


/**
 * @brief helper function to factorize getting any kind of plugin
 *
 * @param plugin_name  The name of the plugin to retrieve
 * @param container    The container where to look for the plugin
 * @param plugins      The instanciated plugins container
 * @param plugin       The plugin
 * @return true on success, false otherwise
 */
template<class PluginType>
bool getPlugin(const std::string &plugin_name,
			   pl_list_t &container,
			   std::vector<OpenSandPlugin *> &plugins,
			   PluginType **plugin)
{
	fn_create create = container[plugin_name].second;
	if(!create)
	{
		return false;
	}

	*plugin = dynamic_cast<PluginType *>(create());
	if(!*plugin)
	{
		return false;
	}

	plugins.push_back(*plugin);
	return true;
}


bool PluginUtils::getEncapsulationPlugin(std::string name,
	                                     EncapPlugin **encapsulation)
{
	return getPlugin(name, this->encapsulation, this->plugins, encapsulation);
};

bool PluginUtils::getSatDelayPlugin(std::string name,
                                    SatDelayPlugin **sat_delay)
{
	return getPlugin(name, this->sat_delay, this->plugins, sat_delay);
};

bool PluginUtils::getLanAdaptationPlugin(std::string name,
	                                     LanAdaptationPlugin **lan_adaptation)
{
	for(std::vector<OpenSandPlugin *>::iterator it = this->plugins.begin();
	    it != this->plugins.end(); ++it)
	{
		if((*it)->getName() == name)
		{
			*lan_adaptation = (LanAdaptationPlugin *)*it;
			return true;
		}
	}
	for(auto& existing_plugin : this->plugins)
	{
		if(existing_plugin->getName() == name)
		{
			*lan_adaptation = static_cast<LanAdaptationPlugin *>(existing_plugin);
			return true;
		}
	}
	return getPlugin(name, this->lan_adaptation, this->plugins, lan_adaptation);
};

bool PluginUtils::getAttenuationPlugin(std::string name,
                                       AttenuationModelPlugin **attenuation)
{
	if(name.size() > 0)
	{
		return getPlugin(name, this->attenuation, this->plugins, attenuation);
	}

	return true;
};


bool PluginUtils::getMinimalConditionPlugin(std::string name,
                                            MinimalConditionPlugin **minimal)
{
	if(name.size() > 0)
	{
		return getPlugin(name, this->minimal, this->plugins, minimal);
	}

	return true;
};


bool PluginUtils::getErrorInsertionPlugin(std::string name,
                                          ErrorInsertionPlugin **error)
{
	if(name.size() > 0)
	{
		return getPlugin(name, this->error, this->plugins, error);
	}

	return true;
};


void PluginUtils::generatePluginsConfiguration(std::shared_ptr<OpenSANDConf::MetaComponent> parent,
	                                           plugin_type_t plugin_type,
	                                           const std::string &parameter_id,
	                                           const std::string &parameter_name,
	                                           const std::string &parameter_description)
{
	pl_list_t *container;

	switch(plugin_type)
	{
		case encapsulation_plugin:
			container = &this->encapsulation;
			break;

		case satdelay_plugin:
			container = &this->sat_delay;
			break;

		case lan_adaptation_plugin:
			container = &this->lan_adaptation;
			break;

		case attenuation_plugin:
			container = &this->attenuation;
			break;

		case minimal_plugin:
			container = &this->minimal;
			break;

		case error_plugin:
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
		fn_configure configure = element.second.first;
		if(!configure)
		{
			continue;
		}
		configure(path, param_id);
	}
}
