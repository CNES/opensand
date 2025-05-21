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
 * @file OpenSandCore.cpp
 * @brief Some OpenSAND core utilities
 */


#include <arpa/inet.h>
#include <cmath>
#include <sstream>

#include "OpenSandCore.h"


std::string getComponentName(Component host)
{
	switch(host)
	{
		case Component::satellite:
			return "sat";
		case Component::gateway:
			return "gw";
		case Component::terminal:
			return "st";
		default:
			return "unknown";
	}
}


AccessType strToAccessType(const std::string& access_type)
{
	if(access_type == "DAMA")
		return AccessType::DAMA;
	else if(access_type == "ACM")
		return AccessType::TDM;
	else if(access_type == "ALOHA")
		return AccessType::ALOHA;
	else if(access_type == "VCM")
		return AccessType::TDM;
	else if(access_type == "SCPC")
		return AccessType::SCPC;
	return AccessType::ERROR;
}


RegenLevel strToRegenLevel(const std::string &regen_level)
{
	if (regen_level == "Transparent")
	{
		return RegenLevel::Transparent;
	}
	else if (regen_level == "BBFrame")
	{
		return RegenLevel::BBFrame;
	}
	else if (regen_level == "IP")
	{
		return RegenLevel::IP;
	}
	else
	{
		return RegenLevel::Unknown;
	}
}


void tokenize(const std::string &str,
              std::vector<std::string> &tokens,
              const std::string& delimiters)
{
	// Skip delimiters at beginning.
	std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	std::string::size_type pos = str.find_first_of(delimiters, last_pos);

	while(std::string::npos != pos || std::string::npos != last_pos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(last_pos, pos - last_pos));
		// Skip delimiters.  Note the "not_of"
		last_pos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, last_pos);
	}
}


uint32_t hcnton(double cn)
{
	int16_t tmp_cn = static_cast<int16_t>(std::round(cn * 100));  // we take two digits in decimal part
	return htonl(static_cast<uint32_t>(tmp_cn));
}


double ncntoh(uint32_t cn)
{
	int16_t tmp_cn = static_cast<int16_t>(ntohl(cn));
	return tmp_cn / 100.0;
}


std::string generateProbePrefix(spot_id_t spot_id, Component entity_type, bool is_sat)
{
	std::ostringstream ss{};
	ss << "spot_" << int{spot_id} << ".";
	if (is_sat)
	{
		ss << "sat.";
	}
	ss << getComponentName(entity_type) << ".";
	return ss.str();
}
