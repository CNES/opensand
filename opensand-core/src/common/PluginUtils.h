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
class IslDelayPlugin;

class OutputLog;

namespace OpenSANDConf {
	class MetaComponent;
}


template<class T>
struct PluginConfigurationElement {
	fn_configure init;
	fn_create create;
	std::shared_ptr<T> plugin;
};
template<class T>
using PluginConfigurationContainer = std::map<std::string, PluginConfigurationElement<T>>;


/**
 * @class PluginUtils
 * @brief Utilities for Plugins
 */
class PluginUtils
{
	friend class Plugin;

protected:
	PluginConfigurationContainer<EncapPlugin> encapsulation;
	PluginConfigurationContainer<LanAdaptationPlugin> lan_adaptation;
	PluginConfigurationContainer<AttenuationModelPlugin> attenuation;
	PluginConfigurationContainer<MinimalConditionPlugin> minimal;
	PluginConfigurationContainer<ErrorInsertionPlugin> error;
	PluginConfigurationContainer<SatDelayPlugin> sat_delay;
	PluginConfigurationContainer<IslDelayPlugin> isl_delay;
	std::vector<void *> handlers;

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
	 * @return the plugin on success, nullptr otherwise
	 */
	std::shared_ptr<EncapPlugin> getEncapsulationPlugin(std::string name);

	/**
	 * @brief get a satellite ISL delay plugin
	 *
	 * @param name           The name of the satellite delay plugin
	 * @return the plugin on success, nullptr otherwise
	 */
	std::shared_ptr<IslDelayPlugin> getIslDelayPlugin(std::string name);

	/**
	 * @brief get a satellite delay plugin
	 *
	 * @param name           The name of the satellite delay plugin
	 * @return the plugin on success, nullptr otherwise
	 */
	std::shared_ptr<SatDelayPlugin> getSatDelayPlugin(std::string name);

	/**
	 * @brief get physical layer attenuation plugin
	 *
	 * @param name  The name of the attenuation model plugin
	 * @return the plugin on success, nullptr otherwise
	 */
	std::shared_ptr<AttenuationModelPlugin> getAttenuationPlugin(std::string name);

	/**
	 * @brief get physical layer minimal condition plugin
	 *
	 * @param name  The name of the minimal condition plugin
	 * @return the plugin on success, nullptr otherwise
	 */
	std::shared_ptr<MinimalConditionPlugin> getMinimalConditionPlugin(std::string name);

	/**
	 * @brief get physical layer error insertion plugin
	 *
	 * @param name  The name of the erroe insertion plugin
	 * @return the plugin on success, nullptr otherwise
	 */
	std::shared_ptr<ErrorInsertionPlugin> getErrorInsertionPlugin(std::string name);

	/**
	 * @brief get the encapsulation plugins list
	 *
	 * @return a vector containing the names of all known encapsulation plugins
	 */
	std::vector<std::string> getAllEncapsulationPlugins();

	void generatePluginsConfiguration(std::shared_ptr<OpenSANDConf::MetaComponent> parent,
	                                  PluginType plugin_type,
	                                  const std::string &parameter_id,
	                                  const std::string &parameter_name,
	                                  const std::string &parameter_description);

	/// the log
	std::shared_ptr<OutputLog> log_init;
};


#endif
