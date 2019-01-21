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
 * @file ModulationTypes.cpp
 * @brief The modulation types
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include "ModulationTypes.h"

ModulationTypes ModulationTypes::instance;

ModulationTypes::ModulationTypes() :
	default_modulation_efficiency(1),
	modulation_efficiencies()
{
	this->modulation_efficiencies["BPSK"] = 1;
	this->modulation_efficiencies["Pi/2BPSK"] = 1;
	this->modulation_efficiencies["QPSK"] = 2;
	this->modulation_efficiencies["8PSK"] = 3;
	this->modulation_efficiencies["16APSK"] = 4;
	this->modulation_efficiencies["16QAM"] = 4;
	this->modulation_efficiencies["32APSK"] = 5;
}

ModulationTypes::~ModulationTypes()
{
}

bool ModulationTypes::exist(string modulation_label)
{
	map<string, unsigned int>::iterator ite;

	ite = instance.modulation_efficiencies.find(modulation_label);
	return (ite != instance.modulation_efficiencies.end());
}

unsigned int ModulationTypes::getDefaultEfficiency()
{
	return instance.default_modulation_efficiency;
}

unsigned int ModulationTypes::getEfficiency(string modulation_label)
{
	map<string, unsigned int>::iterator ite;

	ite = instance.modulation_efficiencies.find(modulation_label);
	if(ite == instance.modulation_efficiencies.end())
	{
		return instance.default_modulation_efficiency;
	}
	return (ite->second);
}
