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
 * @file Path.cpp
 * @brief Provides util functions to handle path.
 */

#include <sstream>

#include "Path.h"


std::vector<std::string> OpenSANDConf::splitPath(const std::string &path, const char &separator)
{
  std::string item;
  std::vector<std::string> split;
        std::stringstream ss(path);

	if(path.empty())
	{
		return split;
	}
        while(std::getline(ss, item, separator))
        {
		if(!item.empty())
		{
			split.push_back(item);
		}
        }
	return split;
}

std::string OpenSANDConf::getCommonPath(const std::string &path1, const std::string &path2, const char &separator)
{
	bool equal;
	unsigned int i, n;
  std::vector<std::string> split1, split2;
	std::stringstream ss;

	// Check equality
	if(path1 == path2)
	{
		return path1;
	}

	// Split paths
	split1 = splitPath(path1, separator);
	split2 = splitPath(path2, separator);
	n = split1.size() <= split2.size() ? split1.size() : split2.size();
	
	// Get greater common path between element and target
	equal = true;
	i = 0;
	while(equal && i < n && split1[i] == split2[i])
	{
		ss << "/" << split1[i];
		++i;
	}
	return ss.str();
}

std::string OpenSANDConf::getRelativePath(const std::string &parentpath, const std::string &path, const char &separator)
{
	if(parentpath.empty())
	{
		return path;
	}
	auto pos = path.find(parentpath);
	if(pos != 0)
	{
		return path;
	}
	pos = parentpath.size();
	while(pos < path.size() && path[pos] == separator)
	{
		++pos;
	}
	return path.substr(pos);
}

bool OpenSANDConf::checkPathId(const std::string &id, const char &separator)
{
	return id.find(separator) == std::string::npos;
}
