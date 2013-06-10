/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @brief  High level interface for opensand plugin utilities
 *
 */


#ifndef OPENSAND_PLUGIN_STATIC_H
#define OPENSAND_PLUGIN_STATIC_H

#include "PluginUtils.h"

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
	 * @brief get a lan adaptation plugin
	 *
	 * @param name           The name of the lan_adaptation plugin
	 * @param lan_adaptation  The lan adaptation plugin
	 * @return true on success, false otherwise
	 */
	static bool getLanAdaptationPlugin(string name,
	                                   LanAdaptationPlugin **lan_adaptation);

	/**
	 * @brief get physical layer plugins
	 *
	 * @param att_pl_name  The name of the attenuation model plugin
	 * @param nom_pl_name  The name of the nominal condition plugin
	 * @param min_pl_name  The name of the minimal condition plugin
	 * @param err_pl_name  The name of the erroe insertion plugin
	 * @param attenuation  The attenuation model plugin
	 * @param nominal      The nominal condition plugin
	 * @param minimal      The minimal condition plugin
	 * @param error        The error insertion plugin
	 * @return true on success, false otherwise
	 */
	static bool getPhysicalLayerPlugins(std::string att_pl_name,
	                                    std::string nom_pl_name,
	                                    std::string min_pl_name,
	                                    std::string err_pl_name,
	                                    AttenuationModelPlugin **attenuation,
	                                    NominalConditionPlugin **nominal,
	                                    MinimalConditionPlugin **minimal,
	                                    ErrorInsertionPlugin **error);

	/**
	 * @brief get the encapsulation plugins list
	 *
	 * @param name           The name of the encapsulation plugin
	 * @param encapsulation  The encapsulation plugin
	 * @return true on success, false otherwise
	 */
	static void getAllEncapsulationPlugins(pl_list_t &encapsulation);

	/**
	 * @brief get the lan adaptation plugins list
	 *
	 * @param encapsulation  The lan adaptation plugins
	 * @return true on success, false otherwise
	 */
	static void getAllLanAdaptationPlugins(pl_list_t &lan_adaptation);


   private:

	/// The block manager instance
	static PluginUtils utils;

};

#endif


