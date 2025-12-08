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
 * @file Plugin.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @author Joaquin MUGUERZA / <jmuguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>
 * @brief  High level interface for opensand-rt
 *
 */


#include "Plugin.h"


// Create Plugin instance
PluginUtils Plugin::utils;


bool Plugin::loadPlugins(bool enable_phy_layer)
{
	return utils.loadPlugins(enable_phy_layer);
}


void Plugin::releasePlugins()
{
	utils.releasePlugins();
}


std::shared_ptr<EncapPlugin> Plugin::getEncapsulationPlugin(std::string name)
{
	return utils.getEncapsulationPlugin(name);
}


std::shared_ptr<AttenuationModelPlugin> Plugin::getAttenuationPlugin(std::string name)
{
	return utils.getAttenuationPlugin(name);
}


std::shared_ptr<MinimalConditionPlugin> Plugin::getMinimalConditionPlugin(std::string name)
{
	return utils.getMinimalConditionPlugin(name);
}


std::shared_ptr<ErrorInsertionPlugin> Plugin::getErrorInsertionPlugin(std::string name)
{
	return utils.getErrorInsertionPlugin(name);
}


std::shared_ptr<SatDelayPlugin> Plugin::getSatDelayPlugin(std::string name)
{
	return utils.getSatDelayPlugin(name);
}


std::shared_ptr<IslDelayPlugin> Plugin::getIslDelayPlugin(std::string name)
{
	return utils.getIslDelayPlugin(name);
}


std::vector<std::string> Plugin::getAllEncapsulationPlugins()
{
	return utils.getAllEncapsulationPlugins();
}


void Plugin::generatePluginsConfiguration(std::shared_ptr<OpenSANDConf::MetaComponent> parent,
                                          PluginType plugin_type,
                                          const std::string &parameter_id,
                                          const std::string &parameter_name,
                                          const std::string &parameter_description)
{
	utils.generatePluginsConfiguration(parent,
	                                   plugin_type,
	                                   parameter_id,
	                                   parameter_name,
	                                   parameter_description);
}
