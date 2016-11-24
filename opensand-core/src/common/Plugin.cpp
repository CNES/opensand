/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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
 * @brief  High level interface for opensand-rt
 *
 */

#include "Plugin.h"

#include <stdarg.h>
#include <cstdio>
#include <algorithm>

// Create Plugin instance
PluginUtils Plugin::utils;


bool Plugin::loadPlugins(bool enable_phy_layer, string conf_path)
{
	return utils.loadPlugins(enable_phy_layer, conf_path);
}

void Plugin::releasePlugins()
{
	utils.releasePlugins();
}


bool Plugin::getEncapsulationPlugin(string name,
                                    EncapPlugin **encapsulation)
{
	return utils.getEncapsulationPlugin(name, encapsulation);
}

bool Plugin::getLanAdaptationPlugin(string name,
                                    LanAdaptationPlugin **lan_adaptation)
{
	return utils.getLanAdaptationPlugin(name, lan_adaptation);
}

bool Plugin::getPhysicalLayerPlugins(string att_pl_name,
                                     string min_pl_name,
                                     string err_pl_name,
                                     AttenuationModelPlugin **attenuation,
                                     MinimalConditionPlugin **minimal,
                                     ErrorInsertionPlugin **error)
{
	return utils.getPhysicalLayerPlugins(att_pl_name,
	                                     min_pl_name,
	                                     err_pl_name,
	                                     attenuation,
	                                     minimal,
	                                     error);
}

void Plugin::getAllEncapsulationPlugins(pl_list_t &encapsulation)
{
	return utils.getAllEncapsulationPlugins(encapsulation);
}

void Plugin::getAllLanAdaptationPlugins(pl_list_t &lan_adaptation)
{
	return utils.getAllLanAdaptationPlugins(lan_adaptation);
}
