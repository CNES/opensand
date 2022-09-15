/*
 *
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
 * @file CodingTypes.cpp
 * @brief The modulation types
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include "CodingTypes.h"


CodingTypes::CodingTypes() :
	default_coding_rate(1),
	coding_rates()
{
	this->coding_rates["1/4"] = 1.0 / 4.0;
	this->coding_rates["1/3"] = 1.0 / 3.0;
	this->coding_rates["2/5"] = 2.0 / 5.0;
	this->coding_rates["1/2"] = 1.0 / 2.0;
	this->coding_rates["3/5"] = 3.0 / 5.0;
	this->coding_rates["2/3"] = 2.0 / 3.0;
	this->coding_rates["3/4"] = 3.0 / 4.0;
	this->coding_rates["4/5"] = 4.0 / 5.0;
	this->coding_rates["5/6"] = 5.0 / 6.0;
	this->coding_rates["6/7"] = 6.0 / 7.0;
	this->coding_rates["8/9"] = 8.0 / 9.0;
	this->coding_rates["9/10"] = 9.0 / 10.0;
}

CodingTypes::~CodingTypes()
{
}

bool CodingTypes::exist(std::string coding_label)
{
	CodingTypes instance;
	std::map<std::string, float>::iterator ite;

	ite = instance.coding_rates.find(coding_label);
	return (ite != instance.coding_rates.end());
}

float CodingTypes::getDefaultRate()
{
	CodingTypes instance;
	return instance.default_coding_rate;
}

float CodingTypes::getRate(std::string coding_label)
{
	CodingTypes instance;
	std::map<std::string, float>::iterator ite;

	ite = instance.coding_rates.find(coding_label);
	if(ite == instance.coding_rates.end())
	{
		return instance.default_coding_rate;
	}
	return (ite->second);
}
