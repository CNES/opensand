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
 * @file PluginUtils.h
 * @brief Utilities for Plugins
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#ifndef PLUGIN_UTILS_HEADER_H
#define PLUGIN_UTILS_HEADER_H


#include "OpenSandPlugin.h"

#include <map>
#include <vector>
#include <string>
#include <utility>
#include <memory>


class EncapPlugin;
class LanAdaptationPlugin;
class AttenuationModelPlugin;
class MinimalConditionPlugin;
class ErrorInsertionPlugin;
class SatDelayPlugin;

class OutputLog;

namespace OpenSANDConf {
	class MetaComponent;
}

typedef std::map<std::string, std::pair<fn_configure, fn_create>> pl_list_t;


/**
 * @class PluginUtils
 * @brief Utilities for Plugins
 */
class PluginUtils
{
	friend class Plugin;

 protected:
	pl_list_t encapsulation;
	pl_list_t lan_adaptation;
	pl_list_t attenuation;
	pl_list_t minimal;
	pl_list_t error;
	pl_list_t sat_delay;
	std::vector<void *> handlers;
	std::vector<OpenSandPlugin *> plugins;

	PluginUtils();

	/**
	 * @brief load the plugins
	 *
	 * @param enable_phy_layer Whether the physical layer is enabled or not
	 * @return true on success, false otherwise
	 */
	bool loadPlugins(bool enable_phy_layer);

	/**
	 * @brief store the plugin in the appropirate container
	 *        Check for duplicates before doing so.
	 *
	 * @param container  The container where to store the plugin
	 * @param plugin     The plugin to store into the container
	 * @param handle     The handle to the library storing the plugin
	 */
	void storePlugin(pl_list_t &container, opensand_plugin_t *plugin, void *handle);

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
	bool getEncapsulationPlugin(std::string name,
	                            EncapPlugin **encapsulation);

	/**
	 * @brief get a satellite delay plugin
	 *
	 * @param name           The name of the satellite delay plugin
	 * @param sat_delay      The satellite delay plugin
	 * @return true on success, false otherwise
	 */
	bool getSatDelayPlugin(std::string name,
	                       SatDelayPlugin **sat_delay);

	/**
	 * @brief get physical layer attenuation plugin
	 *
	 * @param att_pl_name  The name of the attenuation model plugin
	 * @param attenuation  The attenuation model plugin
	 * @return true on success, false otherwise
	 */
	bool getAttenuationPlugin(std::string att_pl_name,
	                          AttenuationModelPlugin **attenuation);

	/**
	 * @brief get physical layer minimal condition plugin
	 *
	 * @param min_pl_name  The name of the minimal condition plugin
	 * @param minimal      The minimal condition plugin
	 * @return true on success, false otherwise
	 */
	bool getMinimalConditionPlugin(std::string min_pl_name,
	                               MinimalConditionPlugin **minimal);

	/**
	 * @brief get physical layer error insertion plugin
	 *
	 * @param err_pl_name  The name of the erroe insertion plugin
	 * @param error        The error insertion plugin
	 * @return true on success, false otherwise
	 */
	bool getErrorInsertionPlugin(std::string err_pl_name,
	                             ErrorInsertionPlugin **error);

	/**
	 * @brief get the encapsulation plugins list
	 *
	 * @param encapsulation  The encapsulation plugins
	 * @return true on success, false otherwise
	 */
	inline void getAllEncapsulationPlugins(pl_list_t &encapsulation)
	{
		encapsulation = this->encapsulation;
	}

	void generatePluginsConfiguration(std::shared_ptr<OpenSANDConf::MetaComponent> parent,
	                                  plugin_type_t plugin_type,
	                                  const std::string &parameter_id,
	                                  const std::string &parameter_name,
	                                  const std::string &parameter_description);

	/// the log
	std::shared_ptr<OutputLog> log_init;
};


#endif

