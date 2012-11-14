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
 * @file PluginUtils.cpp
 * @brief utilities for Plugins
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#include "OpenSandPlugin.h"
#include "PluginUtils.h"

#include <dirent.h>
#include <errno.h>
#include <dlfcn.h>

#include <opensand_conf/uti_debug.h>

#define PLUGIN_DIRECTORY "/opensand/plugins/"



bool PluginUtils::loadPlugins(bool enable_phy_layer)
{
	DIR *plugin_dir;
	char *lib_path;
	std::vector<std::string> path;

	lib_path = getenv("LD_LIBRARY_PATH");
	if(lib_path)
	{
		// Split using ':' separator
		PluginUtils::tokenize(lib_path, path, ":");
	}
	path.push_back("/usr/lib/");
	path.push_back("/lib");

	for(std::vector<std::string>::iterator iter = path.begin();
	    iter != path.end(); ++iter)
	{
		struct dirent *ent;
		std::string dir = *iter + PLUGIN_DIRECTORY;
		plugin_dir = opendir(dir.c_str());
		if(!plugin_dir)
		{
			UTI_INFO("cannot search plugins in %s folder\n", dir.c_str());
			continue;
		}
		UTI_INFO("search for plugins in %s folder\n", dir.c_str());

		while((ent = readdir(plugin_dir)) != NULL)
		{
			std::string filename = ent->d_name;
			std::string libend = ".so.0";
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
				std::string plugin_name = dir + filename;

				UTI_DEBUG("find plugin library %s\n", filename.c_str());
				handle = dlopen(plugin_name.c_str(), RTLD_LAZY);
				if(!handle)
				{
					UTI_ERROR("cannot load plugin %s (%s)\n",
					          filename.c_str(), dlerror());
					continue;
				}

				sym = dlsym(handle, "init");
				if(!sym)
				{
					UTI_ERROR("cannot find 'init' method in plugin %s (%s)\n",
					          filename.c_str(), dlerror());
					dlclose(handle);
					goto close;
				}
				init = reinterpret_cast<fn_init *>(sym);

				plugin = init();
				if(!plugin)
				{
					UTI_ERROR("cannot create plugin\n");
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
							UTI_INFO("load encapsulation plugin %s\n",
							         plugin->name);
							this->encapsulation[plugin->name] = plugin->create;
						}
						else
						{
							dlclose(handle);
						}
						this->handlers.push_back(handle);
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
							UTI_INFO("load attenuation model plugin %s\n",
							         plugin->name);
							this->attenuation[plugin->name] = plugin->create;
						}
						else
						{
							dlclose(handle);
						}
						this->handlers.push_back(handle);
					}
					break;

					case nominal_plugin:
					{
						if(!enable_phy_layer)
						{
							dlclose(handle);
							break;
						}

						pl_list_it_t plug;

						// if we load twice the same plugin, keep the first one
						// this is why LD_LIBRARY_PATH should be first in the paths
						plug = this->nominal.find(plugin->name);
						if(plug == this->nominal.end())
						{
							UTI_INFO("load nominal conditions plugin %s\n",
							         plugin->name);
							this->nominal[plugin->name] = plugin->create;
						}
						else
						{
							dlclose(handle);
						}
						this->handlers.push_back(handle);
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
							UTI_INFO("load minimal conditions plugin %s\n",
							         plugin->name);
							this->minimal[plugin->name] = plugin->create;
						}
						else
						{
							dlclose(handle);
						}
						this->handlers.push_back(handle);
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
							UTI_INFO("load error insertions plugin %s\n",
							         plugin->name);
							this->error[plugin->name] = plugin->create;
						}
						else
						{
							dlclose(handle);
						}
						this->handlers.push_back(handle);
					}
					break;

					default:
						UTI_ERROR("Wrong plugin type %d for %s",
						          plugin->type, filename.c_str());
				}
				delete plugin;
			}
		}
		closedir(plugin_dir);
	}

	return mgl_ok;
close:
	if(plugin_dir)
	{
		closedir(plugin_dir);
	}
	return mgl_ko;
}


void PluginUtils::releasePlugins()
{
	for(std::vector<OpenSandPlugin *>::iterator iter = this->plugins.begin();
	    iter != this->plugins.end(); ++iter)
	{
		delete *iter;
	}

	for(std::vector<void *>::iterator iter = this->handlers.begin();
	    iter != this->handlers.end(); ++iter)
	{
		dlclose(*iter);
	}
}

bool PluginUtils::getEncapsulationPlugins(std::string name,
	                                      EncapPlugin **encapsulation)
{
	fn_create create;

	create = this->encapsulation[name];
	if(!create)
	{
		return false;
	}
	*encapsulation = dynamic_cast<EncapPlugin *>(create());
	if(*encapsulation == NULL)
	{
		return false;
	}
	plugins.push_back(*encapsulation);

	return true;
};

bool PluginUtils::getPhysicalLayerPlugins(std::string att_pl_name,
										  std::string nom_pl_name,
										  std::string min_pl_name,
										  std::string err_pl_name,
										  AttenuationModelPlugin **attenuation,
										  NominalConditionPlugin **nominal,
										  MinimalConditionPlugin **minimal,
										  ErrorInsertionPlugin **error)
{
	fn_create create;

	create = this->attenuation[att_pl_name];
	if(!create)
	{
		UTI_ERROR("cannot load attenuation model plugin: %s", att_pl_name.c_str());
		return false;
	}
	*attenuation = dynamic_cast<AttenuationModelPlugin *>(create());
	if(*attenuation == NULL)
	{
		UTI_ERROR("cannot create attenuation model plugin: %s", att_pl_name.c_str());
		return false;
	}
	plugins.push_back(*attenuation);

	create = this->nominal[nom_pl_name];
	if(!create)
	{
		UTI_ERROR("cannot load nominal condition plugin: %s", nom_pl_name.c_str());
		return false;
	}
	*nominal = dynamic_cast<NominalConditionPlugin *>(create());
	if(*nominal == NULL)
	{
		UTI_ERROR("cannot create nominal condition plugin: %s", nom_pl_name.c_str());
		return false;
	}
	plugins.push_back(*nominal);

	if(min_pl_name.size() > 0)
	{
		create = this->minimal[min_pl_name];
		if(!create)
		{
			UTI_ERROR("cannot load minimal condition plugin: %s", min_pl_name.c_str());
			return false;
		}
		*minimal = dynamic_cast<MinimalConditionPlugin *>(create());
		if(*minimal == NULL)
		{
			UTI_ERROR("cannot create minimal condition plugin: %s", min_pl_name.c_str());
			return false;
		}
		plugins.push_back(*minimal);
	}


	if(err_pl_name.size() > 0)
	{
		create = this->error[err_pl_name];
		if(!create)
		{
			UTI_ERROR("cannot load error insertion plugin: %s", err_pl_name.c_str());
			return false;
		}
		*error = dynamic_cast<ErrorInsertionPlugin *>(create());
		if(*error == NULL)
		{
			UTI_ERROR("cannot error insertion model plugin: %s", err_pl_name.c_str());
			return false;
		}
		plugins.push_back(*error);
	}

	return true;
};

void PluginUtils::tokenize(const std::string &str,
                           std::vector<std::string> &tokens,
                           const std::string& delimiters)
{
	// Skip delimiters at beginning.
	std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	std::string::size_type pos = str.find_first_of(delimiters, lastPos);

	while(std::string::npos != pos || std::string::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
}
