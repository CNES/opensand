/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 CNES
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
 * @file PluginUtils.h
 * @brief Utilities for Plugins
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#ifndef PLUGIN_UTILS_HEADER_H
#define PLUGIN_UTILS_HEADER_H

#include "EncapPlugin.h"
#include "LanAdaptationPlugin.h"
#include "PhysicalLayerPlugin.h"

#include <map>
#include <vector>
#include <string>


using std::map;
using std::vector;
using std::string;

typedef map<string, fn_create> pl_list_t;
typedef map<string, fn_create>::const_iterator pl_list_it_t;


/**
 * @class PluginUtils
 * @brief Utilities for Plugins
 */
class PluginUtils
{
  protected:

	pl_list_t encapsulation;
	pl_list_t lan_adaptation;
	pl_list_t attenuation;
	pl_list_t minimal;
	pl_list_t error;
	vector <void *> handlers;
	vector<OpenSandPlugin *> plugins;

  public:

	PluginUtils();

	/**
	 * @brief load the plugins
	 *
	 * @param enable_phy_layer Whether the physical layer is enabled or not
	 * @return true on success, false otherwise
	 */
	bool loadPlugins(bool enable_phy_layer);

	/**
	 * @brief release the class elements for plugins
	 */
	void releasePlugins();

	/**
	 * @brief get an encapsulation plugin
	 *
	 * @param name           The name of the encapsulation plugin
	 * @param encapsulation  The encapsulation plugin
	 * @return true on success, false otherwise
	 */
	bool getEncapsulationPlugin(string name,
	                            EncapPlugin **encapsulation);

	/**
	 * @brief get a lan adaptation plugin
	 *
	 * @param name           The name of the lan_adaptation plugin
	 * @param lan_adaptation  The lan adaptation plugin
	 * @return true on success, false otherwise
	 */
	bool getLanAdaptationPlugin(string name,
	                            LanAdaptationPlugin **lan_adaptation);

	/**
	 * @brief get physical layer plugins
	 *
	 * @param att_pl_name  The name of the attenuation model plugin
	 * @param min_pl_name  The name of the minimal condition plugin
	 * @param err_pl_name  The name of the erroe insertion plugin
	 * @param attenuation  The attenuation model plugin
	 * @param minimal      The minimal condition plugin
	 * @param error        The error insertion plugin
	 * @return true on success, false otherwise
	 */
	bool getPhysicalLayerPlugins(string att_pl_name,
	                             string min_pl_name,
	                             string err_pl_name,
	                             AttenuationModelPlugin **attenuation,
	                             MinimalConditionPlugin **minimal,
	                             ErrorInsertionPlugin **error);

	/**
	 * @brief get the encapsulation plugins list
	 *
	 * @param encapsulation  The encapsulation plugins
	 * @return true on success, false otherwise
	 */
	void getAllEncapsulationPlugins(pl_list_t &encapsulation)
	{
		encapsulation = this->encapsulation;
	}

	/**
	 * @brief get the lan adaptation plugins list
	 *
	 * @param encapsulation  The lan adaptation plugins
	 * @return true on success, false otherwise
	 */
	void getAllLanAdaptationPlugins(pl_list_t &lan_adaptation)
	{
		lan_adaptation = this->lan_adaptation;
	}

};


#endif

