/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 CNES
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


#include "PluginUtils.h"

#include <dirent.h>
#include <errno.h>
#include <dlfcn.h>

#include <opensand_conf/uti_debug.h>

#define PLUGIN_DIRECTORY "/opensand/plugins/"



bool PluginUtils::loadEncapPlugins(std::map<std::string, EncapPlugin *> &encap_plug)
{
	const char *FUNCNAME = "[loadEncapPlugins]";
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
				fn_create *create;
				EncapPlugin *plugin;
				std::map<std::string, EncapPlugin *>::iterator plug;

				std::string plugin_name = dir + filename;

				UTI_DEBUG("%s find plugin library %s\n", FUNCNAME, filename.c_str());
				handle = dlopen(plugin_name.c_str(), RTLD_LAZY);
				if(!handle)
				{
					UTI_ERROR("%s cannot load plugin %s (%s)\n", FUNCNAME,
					          filename.c_str(), dlerror());
					continue;
				}

				sym = dlsym(handle, "create");
				if(!sym)
				{
					UTI_ERROR("%s cannot find 'create' method in plugin %s (%s)\n",
					          FUNCNAME, filename.c_str(), dlerror());
					dlclose(handle);
					goto close;
				}
				create = reinterpret_cast<fn_create *>(sym);

				plugin = create();
				if(!plugin)
				{
					UTI_ERROR("cannot create plugin\n");
					continue;
				}

				// if we load twice the same plugin, keep the first one
				// this is why LD_LIBRARY_PATH should be first in the paths
				plug = encap_plug.find(plugin->getName());
				if(plug == encap_plug.end())
				{
					UTI_INFO("load plugin %s\n", plugin->getName().c_str());
					encap_plug[plugin->getName()] = plugin;
				}
				else
				{
					dlclose(handle);
				}
				this->handlers.push_back(handle);
			}
		}
		closedir(plugin_dir);
	}
	this->encap_plug = encap_plug;

	return mgl_ok;
close:
	if(plugin_dir)
	{
		closedir(plugin_dir);
	}
	return mgl_ko;
}


void PluginUtils::releaseEncapPlugins()
{
	for(std::map<std::string, EncapPlugin *>::iterator iter = this->encap_plug.begin();
	    iter != this->encap_plug.end(); ++iter)
	{
		delete (*iter).second;
	}
	for(std::vector<void *>::iterator iter = this->handlers.begin();
	    iter != this->handlers.end(); ++iter)
	{
		dlclose(*iter);
	}
}

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
