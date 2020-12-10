/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 Viveris Technologies
 * Copyright © 2020 TAS
 * Copyright © 2020 CNES
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
 * @file Path.h
 * @brief Provides util functions to handle path.
 */

#ifndef OPENSAND_CONF_PATH_H
#define OPENSAND_CONF_PATH_H

#include <string>
#include <vector>


namespace OpenSANDConf
{
	/**
	 * @brief Split a path to a vector of ids, using a specific separator.
	 *
	 * @param  path       The path to split
	 * @param  separator  The separator to use
	 *
	 * @return The vector of ids
	 */
  std::vector<std::string> splitPath(const std::string &path, const char &separator='/');

	/**
	 * @brief Get the common path shared by two paths, using a specific separator.
	 *
	 * @param  path1      The first path
	 * @param  path2      The second path
	 * @param  separator  The separator to use
	 *
	 * @return The common path shared by path1 and path2
	 */
  std::string getCommonPath(const std::string &path1, const std::string &path2, const char &separator='/');

	/**
	 * @brief Get the relative path based on a parent path, using a specific separator.
	 *
	 * @param  parentpath  The parent path used as base of the returned relative path
	 * @param  path        The path to return the relative path
	 * @param  separator   The separator to use
	 *
	 * @return The relative path of the passed path regarding the parentpath
	 */
  std::string getRelativePath(const std::string &parentpath, const std::string &path, const char &separator='/');

	/**
	 * @brief Check a string is a valid id for a path, using a specific separator.
	 *
	 * @param  id         The string to check
	 * @param  separator  The separator to use
	 *
	 * @return True if the string is a valid path id, False otherwise
	 */
	bool checkPathId(const std::string &id, const char &separator='/');
}

#endif // OPENSAND_CONF_PATH_H
