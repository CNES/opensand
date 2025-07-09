/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file Plugin.h
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @author Joaquin MUGUERZA / <jmuguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>
 * @brief  High level interface for opensand plugin utilities
 *
 */


#ifndef OPENSAND_PLUGIN_STATIC_H
#define OPENSAND_PLUGIN_STATIC_H


#include "PluginUtils.h"

#include <memory>


/**
 * @class Plugin
 *
 * @brief The interface for opensand-rt
 */
class Plugin
{
public:
	Plugin();
	~Plugin();

	/**
	 * @brief load the plugins
	 *
	 * @param enable_phy_layer Whether the physical layer is enabled or not
	 * @return true on success, false otherwise
	 */
	static bool loadPlugins(bool enable_phy_layer);

	/**
	 * @brief release the class elements for plugins
	 */
	static void releasePlugins();

	/**
	 * @brief get a encapsulation plugins
	 *
	 * @param name           The name of the encapsulation plugin
	 * @param encapsulation  The encapsulation plugin
	 * @return true on success, false otherwise
	 */
	static bool getEncapsulationPlugin(std::string name,
	                                   EncapPlugin **encapsulation);

	/**
	 * @brief get physical layer attenuation plugin
	 *
	 * @param att_pl_name  The name of the attenuation model plugin
	 * @param attenuation  The attenuation model plugin
	 * @return true on success, false otherwise
	 */
	static bool getAttenuationPlugin(std::string att_pl_name,
	                                 AttenuationModelPlugin **attenuation);

	/**
	 * @brief get physical layer minimal condition plugin
	 *
	 * @param min_pl_name  The name of the minimal condition plugin
	 * @param minimal      The minimal condition plugin
	 * @return true on success, false otherwise
	 */
	static bool getMinimalConditionPlugin(std::string min_pl_name,
	                                      MinimalConditionPlugin **minimal);

	/**
	 * @brief get physical layer error insertion plugin
	 *
	 * @param err_pl_name  The name of the erroe insertion plugin
	 * @param error        The error insertion plugin
	 * @return true on success, false otherwise
	 */
	static bool getErrorInsertionPlugin(std::string err_pl_name,
	                                    ErrorInsertionPlugin **error);

	/**
	 * @brief get a satellite delay plugin
	 *
	 * @param name           The name of the satellite delay plugin
	 * @param sat_delay      The satellite delay plugin
	 * @return true on success, false otherwise
	 */
	static bool getSatDelayPlugin(std::string name,
	                              SatDelayPlugin **sat_delay);

	/**
	 * @brief get a satellite ISL delay plugin
	 *
	 * @param name           The name of the satellite delay plugin
	 * @param sat_delay      The satellite ISL delay plugin
	 * @return true on success, false otherwise
	 */
	static bool getIslDelayPlugin(std::string name,
	                              IslDelayPlugin **sat_delay);

	/**
	 * @brief get the encapsulation plugins list
	 *
	 * @return a vector containing the names of all known encapsulation plugins
	 */
	static std::vector<std::string> getAllEncapsulationPlugins();

	static void generatePluginsConfiguration(std::shared_ptr<OpenSANDConf::MetaComponent> parent,
	                                         PluginType plugin_type,
	                                         const std::string &parameter_id,
	                                         const std::string &parameter_name,
	                                         const std::string &parameter_description = "");

private:
	/// The block manager instance
	static PluginUtils utils;
};


#endif
