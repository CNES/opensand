/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 CNES
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
 */


#include "OpenSandPlugin.h"
#include "PluginUtils.h"

#include <dirent.h>
#include <errno.h>
#include <dlfcn.h>

#include <opensand_output/Output.h>

#define PLUGIN_DIRECTORY "/opensand/plugins/"


PluginUtils::PluginUtils()
{
}

bool PluginUtils::loadPlugins(bool enable_phy_layer, string conf_path)
{
	DIR *plugin_dir;
	char *lib_path;
	vector<string> path;
	this->log_init = Output::registerLog(LEVEL_WARNING, "init");
	this->conf_path = conf_path;

	lib_path = getenv("LD_LIBRARY_PATH");
	if(lib_path)
	{
		// Split using ':' separator
		tokenize(lib_path, path, ":");
	}
	path.push_back("/usr/lib/");
	path.push_back("/lib");

	for(vector<string>::iterator iter = path.begin();
	    iter != path.end(); ++iter)
	{
		struct dirent *ent;
		string dir = *iter + PLUGIN_DIRECTORY;
		plugin_dir = opendir(dir.c_str());
		if(!plugin_dir)
		{
			LOG(this->log_init, LEVEL_NOTICE,
			    "cannot search plugins in %s folder\n", 
			    dir.c_str());
			continue;
		}
		LOG(this->log_init, LEVEL_NOTICE,
		    "search for plugins in %s folder\n", dir.c_str());

		while((ent = readdir(plugin_dir)) != NULL)
		{
			string filename = ent->d_name;
			string libend = ".so.0";
			if(filename.length() <= libend.length())
			{
				continue;
			}
			if(!filename.compare(filename.length() - libend.length(),
			                     libend.length(), libend))
			{
				void *handle;
				void *sym;
				fn_init *init;
				opensand_plugin_t *plugin;
				string plugin_name = dir + filename;

				LOG(this->log_init, LEVEL_INFO,
				    "find plugin library %s\n", filename.c_str());
				handle = dlopen(plugin_name.c_str(), RTLD_LAZY);
				if(!handle)
				{
					LOG(this->log_init, LEVEL_ERROR,
					    "cannot load plugin %s (%s)\n",
					    filename.c_str(), dlerror());
					continue;
				}

				sym = dlsym(handle, "init");
				if(!sym)
				{
					LOG(this->log_init, LEVEL_ERROR,
					    "cannot find 'init' method in plugin %s "
					    "(%s)\n", filename.c_str(), dlerror());
					dlclose(handle);
					goto close;
				}
				init = reinterpret_cast<fn_init *>(sym);

				plugin = init();
				if(!plugin)
				{
					LOG(this->log_init, LEVEL_ERROR,
					    "cannot create plugin\n");
					continue;
				}

				switch(plugin->type)
				{
					case encapsulation_plugin:
					{
						pl_list_it_t plug;

						// if we load twice the same plugin, keep the first one
						// this is why LD_LIBRARY_PATH should be first in the paths
						plug = this->encapsulation.find(plugin->name);
						if(plug == this->encapsulation.end())
						{
							LOG(this->log_init, LEVEL_NOTICE,
							    "load encapsulation plugin %s\n",
							    plugin->name.c_str());
							this->encapsulation[plugin->name] = plugin->create;
							this->handlers.push_back(handle);
						}
						else
						{
							dlclose(handle);
						}
					}
					break;

					case lan_adaptation_plugin:
					{
						pl_list_it_t plug;

						// if we load twice the same plugin, keep the first one
						// this is why LD_LIBRARY_PATH should be first in the paths
						plug = this->lan_adaptation.find(plugin->name);
						if(plug == this->lan_adaptation.end())
						{
							LOG(this->log_init, LEVEL_NOTICE,
							    "load lan adaptation plugin %s\n",
							    plugin->name.c_str());
							this->lan_adaptation[plugin->name] = plugin->create;
							this->handlers.push_back(handle);
						}
						else
						{
							dlclose(handle);
						}
					}
					break;

					case attenuation_plugin:
					{
						if(!enable_phy_layer)
						{
							dlclose(handle);
							break;
						}

						pl_list_it_t plug;

						// if we load twice the same plugin, keep the first one
						// this is why LD_LIBRARY_PATH should be first in the paths
						plug = this->attenuation.find(plugin->name);
						if(plug == this->attenuation.end())
						{
							LOG(this->log_init, LEVEL_NOTICE,
							    "load attenuation model plugin "
							    "%s\n", plugin->name.c_str());
							this->attenuation[plugin->name] = plugin->create;
							this->handlers.push_back(handle);
						}
						else
						{
							dlclose(handle);
						}
					}
					break;

					case minimal_plugin:
					{
						if(!enable_phy_layer)
						{
							dlclose(handle);
							break;
						}

						pl_list_it_t plug;

						// if we load twice the same plugin, keep the first one
						// this is why LD_LIBRARY_PATH should be first in the paths
						plug = this->minimal.find(plugin->name);
						if(plug == this->minimal.end())
						{
							LOG(this->log_init, LEVEL_NOTICE,
							    "load minimal conditions plugin"
							    " %s\n", plugin->name.c_str());
							this->minimal[plugin->name] = plugin->create;
							this->handlers.push_back(handle);
						}
						else
						{
							dlclose(handle);
						}
					}
					break;

					case error_plugin:
					{
						if(!enable_phy_layer)
						{
							dlclose(handle);
							break;
						}

						pl_list_it_t plug;

						// if we load twice the same plugin, keep the first one
						// this is why LD_LIBRARY_PATH should be first in the paths
						plug = this->error.find(plugin->name);
						if(plug == this->error.end())
						{
							LOG(this->log_init, LEVEL_NOTICE,
							    "load error insertions plugin "
							    "%s\n", plugin->name.c_str());
							this->error[plugin->name] = plugin->create;
							this->handlers.push_back(handle);
						}
						else
						{
							dlclose(handle);
						}
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
close:
	if(plugin_dir)
	{
		closedir(plugin_dir);
	}
	return false;
}


void PluginUtils::releasePlugins()
{
	for(vector<OpenSandPlugin *>::iterator iter = this->plugins.begin();
	    iter != this->plugins.end(); ++iter)
	{
		delete (*iter);
	}

	for(vector<void *>::iterator iter = this->handlers.begin();
	    iter != this->handlers.end(); ++iter)
	{
		dlclose(*iter);
	}
}

bool PluginUtils::getEncapsulationPlugin(string name,
	                                     EncapPlugin **encapsulation)
{
	fn_create create;

	create = this->encapsulation[name];
	if(!create)
	{
		return false;
	}
	*encapsulation = dynamic_cast<EncapPlugin *>(create(this->conf_path));
	if(*encapsulation == NULL)
	{
		return false;
	}
	this->plugins.push_back(*encapsulation);

	return true;
};

bool PluginUtils::getLanAdaptationPlugin(string name,
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

	fn_create create;

	create = this->lan_adaptation[name];
	if(!create)
	{
		return false;
	}
	*lan_adaptation = dynamic_cast<LanAdaptationPlugin *>(create(this->conf_path));
	if(*lan_adaptation == NULL)
	{
		return false;
	}
	this->plugins.push_back(*lan_adaptation);

	return true;
};

bool PluginUtils::getPhysicalLayerPlugins(string att_pl_name,
                                          string min_pl_name,
                                          string err_pl_name,
                                          AttenuationModelPlugin **attenuation,
                                          MinimalConditionPlugin **minimal,
                                          ErrorInsertionPlugin **error)
{
	fn_create create;

	if(att_pl_name.size() > 0)
	{
		create = this->attenuation[att_pl_name];
		if(!create)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot load attenuation model plugin: %s",
			    att_pl_name.c_str());
			return false;
		}
		*attenuation = dynamic_cast<AttenuationModelPlugin *>(create(this->conf_path));
		if(*attenuation == NULL)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot create attenuation model plugin: %s",
			    att_pl_name.c_str());
			return false;
		}
		this->plugins.push_back(*attenuation);
	}

	if(min_pl_name.size() > 0)
	{
		create = this->minimal[min_pl_name];
		if(!create)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot load minimal condition plugin: %s",
			    min_pl_name.c_str());
			return false;
		}
		*minimal = dynamic_cast<MinimalConditionPlugin *>(create(this->conf_path));
		if(*minimal == NULL)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot create minimal condition plugin: %s",
			    min_pl_name.c_str());
			return false;
		}
		this->plugins.push_back(*minimal);
	}

	if(err_pl_name.size() > 0)
	{
		create = this->error[err_pl_name];
		if(!create)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot load error insertion plugin: %s",
			    err_pl_name.c_str());
			return false;
		}
		*error = dynamic_cast<ErrorInsertionPlugin *>(create(this->conf_path));
		if(*error == NULL)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot error insertion model plugin: %s",
			    err_pl_name.c_str());
			return false;
		}
		this->plugins.push_back(*error);
	}

	return true;
};


