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
#include "LanAdaptationPlugin.h"
#include "PhysicalLayerPlugin.h"
#include "IslPlugin.h"
#include "EncapPlugin.h"
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

template<class T, typename = std::enable_if<std::is_base_of<OpenSandPlugin, T>::value>>
inline bool storePlugin(PluginConfigurationContainer<T> &container, OpenSandPluginFactory *plugin)
{
	// if we load twice the same plugin, keep the first one
	// this is why LD_LIBRARY_PATH should be first in the paths
	const auto [_, inserted] = container.insert({
			plugin->name, {
			plugin->configure,
			plugin->create,
			nullptr}});

	return inserted;
}

bool PluginUtils::loadPlugins(bool enable_phy_layer)
{
	std::vector<std::string> path;
	this->log_init = Output::Get()->registerLog(LEVEL_WARNING, "init");

	char *lib_path = getenv("LD_LIBRARY_PATH");
	if (lib_path)
	{
		// Split using ':' separator
		tokenize(lib_path, path, ":");
	}
	path.push_back(PLUGIN_LIBDIR);

	for (auto &directory : path)
	{
		std::string dir = directory + PLUGIN_DIRECTORY;

		DIR *plugin_dir = opendir(dir.c_str());
		if (!plugin_dir)
		{
			LOG(this->log_init, LEVEL_NOTICE,
				"cannot search plugins in %s folder\n",
				dir.c_str());
			continue;
		}
		LOG(this->log_init, LEVEL_NOTICE,
			"search for plugins in %s folder\n", dir.c_str());

		struct dirent *ent;
		while ((ent = readdir(plugin_dir)) != NULL)
		{
			std::string filename = ent->d_name;
			if (filename.length() <= PLUGIN_FILE_END.length())
			{
				continue;
			}
			if (!filename.compare(filename.length() - PLUGIN_FILE_END.length(),
								  PLUGIN_FILE_END.length(),
								  PLUGIN_FILE_END))
			{
				std::string plugin_name = dir + filename;

				LOG(this->log_init, LEVEL_INFO,
						"find plugin library %s\n", filename.c_str());
				void *handle = dlopen(plugin_name.c_str(), RTLD_LAZY);
				if (!handle)
				{
					LOG(this->log_init, LEVEL_ERROR,
							"cannot load plugin %s (%s)\n",
							filename.c_str(), dlerror());
					continue;
				}

				void *sym = dlsym(handle, "init");
				if (!sym)
				{
					LOG(this->log_init, LEVEL_ERROR,
							"cannot find 'init' method in plugin %s "
							"(%s)\n",
							filename.c_str(), dlerror());
					dlclose(handle);
					closedir(plugin_dir);
					return false;
				}

				OpenSandPluginFactory *plugin = reinterpret_cast<fn_init *>(sym)();
				if (!plugin)
				{
					LOG(this->log_init, LEVEL_ERROR,
							"cannot create plugin\n");
					dlclose(handle);
					continue;
				}

				bool inserted = false;
				switch (plugin->type)
				{
					case PluginType::Encapsulation:
						inserted = storePlugin(this->encapsulation, plugin);
						break;

					case PluginType::IslDelay:
						inserted = storePlugin(this->isl_delay, plugin);
						break;

					case PluginType::SatDelay:
						inserted = storePlugin(this->sat_delay, plugin);
						break;

					case PluginType::Attenuation:
						if (enable_phy_layer)
						{
							inserted = storePlugin(this->attenuation, plugin);
						}
						break;

					case PluginType::Minimal:
						if (enable_phy_layer)
						{
							inserted = storePlugin(this->minimal, plugin);
						}
						break;

					case PluginType::Error:
						if (enable_phy_layer)
						{
							inserted = storePlugin(this->error, plugin);
						}
						break;

					default:
						LOG(this->log_init, LEVEL_ERROR,
								"Wrong plugin type %d for %s",
								plugin->type, filename.c_str());
				}
				delete plugin;

				if (inserted)
				{
					LOG(this->log_init, LEVEL_NOTICE,
							"load plugin %s\n",
							plugin->name.c_str());
					this->handlers.push_back(handle);
				}
				else
				{
					dlclose(handle);
				}
			}
		}
		closedir(plugin_dir);
	}

	return true;
}

void PluginUtils::releasePlugins()
{
	encapsulation.clear();
	lan_adaptation.clear();
	attenuation.clear();
	minimal.clear();
	error.clear();
	sat_delay.clear();
	isl_delay.clear();

	for (auto &&handler : this->handlers)
	{
		dlclose(handler);
	}
	handlers.clear();
}

/**
 * @brief helper function to factorize getting any kind of plugin
 *
 * @param plugin_name  The name of the plugin to retrieve
 * @param container    The container where to look for the plugin
 * @param plugin       The plugin
 * @return true on success, false otherwise
 */
template <class PluginType>
std::shared_ptr<PluginType> getPlugin(
		std::shared_ptr<OutputLog> log,
		const std::string &plugin_name,
		PluginConfigurationContainer<PluginType> &container)
{
	auto plugin_configuration = container.find(plugin_name);
	if (plugin_configuration == container.end())
	{
		LOG(log, LEVEL_ERROR, "Can not find plugin %s", plugin_name.c_str());
		return nullptr;
	}

	PluginConfigurationElement<PluginType> &configuration = plugin_configuration->second;
	if (configuration.plugin != nullptr)
	{
		return configuration.plugin;
	}

	if (!configuration.create)
	{
		LOG(log, LEVEL_ERROR, "No create function found for plugin %s", plugin_name.c_str());
		return nullptr;
	}

	OpenSandPlugin *plugin = configuration.create();
	if (!plugin)
	{
		LOG(log, LEVEL_ERROR, "Create returned nullptr in %s", plugin_name.c_str());
		return nullptr;
	}

	PluginType *plugin_cast = dynamic_cast<PluginType *>(plugin);
	if (!plugin_cast)
	{
		LOG(log, LEVEL_ERROR, "Cast to proper type failed in %s", plugin_name.c_str());
		delete plugin;
		return nullptr;
	}

	configuration.plugin = std::shared_ptr<PluginType>(plugin_cast);
	return configuration.plugin;
}

std::shared_ptr<EncapPlugin> PluginUtils::getEncapsulationPlugin(std::string name)
{
	return getPlugin(this->log_init, name, this->encapsulation);
};

std::shared_ptr<IslDelayPlugin> PluginUtils::getIslDelayPlugin(std::string name)
{
	return getPlugin(this->log_init, name, this->isl_delay);
};

std::shared_ptr<SatDelayPlugin> PluginUtils::getSatDelayPlugin(std::string name)
{
	return getPlugin(this->log_init, name, this->sat_delay);
};

std::shared_ptr<AttenuationModelPlugin> PluginUtils::getAttenuationPlugin(std::string name)
{
	return getPlugin(this->log_init, name, this->attenuation);
};

std::shared_ptr<MinimalConditionPlugin> PluginUtils::getMinimalConditionPlugin(std::string name)
{
	return getPlugin(this->log_init, name, this->minimal);
};

std::shared_ptr<ErrorInsertionPlugin> PluginUtils::getErrorInsertionPlugin(std::string name)
{
	return getPlugin(this->log_init, name, this->error);
};

template<class T>
void _generatePluginsConfiguration(
		std::shared_ptr<OpenSANDConf::MetaComponent> parent,
		const PluginConfigurationContainer<T>& container,
		const std::string& parameter_id,
		const std::string& parameter_name,
		const std::string& parameter_description)
{
	const std::string type_name = std::string{"plugin_"} + parameter_id;
	std::vector<std::string> plugin_names;
	plugin_names.reserve(container.size());
	for (auto const &element : container)
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
	for (auto const &element : container)
	{
		fn_configure configure = element.second.init;
		if (configure)
		{
			configure(path, param_id);
		}
	}
}

void PluginUtils::generatePluginsConfiguration(std::shared_ptr<OpenSANDConf::MetaComponent> parent,
											   PluginType plugin_type,
											   const std::string &parameter_id,
											   const std::string &parameter_name,
											   const std::string &parameter_description)
{
	switch (plugin_type)
	{
	case PluginType::Encapsulation:
		_generatePluginsConfiguration(parent, this->encapsulation, parameter_id, parameter_name, parameter_description);
		break;

	case PluginType::IslDelay:
		_generatePluginsConfiguration(parent, this->isl_delay, parameter_id, parameter_name, parameter_description);
		break;

	case PluginType::SatDelay:
		_generatePluginsConfiguration(parent, this->sat_delay, parameter_id, parameter_name, parameter_description);
		break;

	case PluginType::Attenuation:
		_generatePluginsConfiguration(parent, this->attenuation, parameter_id, parameter_name, parameter_description);
		break;

	case PluginType::Minimal:
		_generatePluginsConfiguration(parent, this->minimal, parameter_id, parameter_name, parameter_description);
		break;

	case PluginType::Error:
		_generatePluginsConfiguration(parent, this->error, parameter_id, parameter_name, parameter_description);
		break;

	default:
		LOG(this->log_init, LEVEL_ERROR,
			"Unable to generate configuration for plugin type %d",
			plugin_type);
		return;
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
