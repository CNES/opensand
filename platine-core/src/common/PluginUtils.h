/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 CNES
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @brief Utilities for Plugins
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#ifndef PLUGIN_UTILS_HEADER_H
#define PLUGIN_UTILS_HEADER_H


#include "EncapPlugin.h"

/**
 * @class PluginUtils
 * @brief Utilities for Plugins
 */
class PluginUtils
{
  protected:

	std::map<std::string, EncapPlugin *> encap_plug;
	std::vector <void *> handlers;

  public:
	/**
	 * @brief load the Encapsulation plugins
	 *
	 * @param encap_plug A map of encapsulation plugins
	 * @return true on success, false otherwise
	 */
	bool loadEncapPlugins(std::map<std::string, EncapPlugin *> &encap_plug);

	/**
	 * @brief release the class elements
	 */
	void releaseEncapPlugins();

	/**
	 * @brief  Tokenize a string
	 *
	 * @param  str        The string to tokenize.
	 * @param  tokens     The list to add tokens into.
	 * @param  delimiter  The tokens' delimiter.
	 */
	static void tokenize(const std::string &str,
	                     std::vector<std::string> &tokens,
	                     const std::string &delimiter=":");
};

#endif

