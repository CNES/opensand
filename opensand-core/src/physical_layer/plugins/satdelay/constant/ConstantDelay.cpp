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
 * @file Constant.cpp
 * @brief Constant
 * @author Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.fr>
 */


#include "ConstantDelay.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>


std::string ConstantDelay::config_path = "";


ConstantDelay::ConstantDelay():
		SatDelayPlugin(),
		is_init(false)
{
}


ConstantDelay::~ConstantDelay()
{  
}


void ConstantDelay::generateConfiguration(const std::string &parent_path,
                                          const std::string &param_id,
                                          const std::string& plugin_name)
{
	auto Conf = OpenSandModelConf::Get();
	auto types = Conf->getModelTypesDefinition();

	ConstantDelay::config_path = parent_path;
	auto delay = Conf->getComponentByPath(parent_path);
	if (delay == nullptr)
	{
		return;
	}
	auto delay_type = delay->getParameter(param_id);
	if (delay_type == nullptr)
	{
		return;
	}

	auto delay_value = delay->addParameter("delay_value", "Delay Value", types->getType("int"));
	delay_value->setUnit("ms");
	Conf->setProfileReference(delay_value, delay_type, plugin_name);
}


bool ConstantDelay::init()
{
	time_ms_t delay_ms;
	auto delay = OpenSandModelConf::Get()->getProfileData(config_path);

	if(this->is_init)
		return true;

	int delay_value;
	auto delay_parameter = delay->getParameter("delay_value");
	if(!OpenSandModelConf::extractParameterData(delay_parameter, delay_value))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'physical_layer', missing parameter 'delay value'\n");
		return false;
	}
	delay_ms = delay_value;

	LOG(this->log_init, LEVEL_DEBUG,
	    "Constant delay: %d ms", delay_ms);
	this->setSatDelay(delay_ms);

	// TODO: should is_init use a mutex??
	this->is_init = true;
	return true;
}


bool ConstantDelay::updateSatDelay()
{
	// Empty function, not necessary for a constant delay
	return true;
}


bool ConstantDelay::getMaxDelay(time_ms_t &delay) const
{
	if(!this->is_init)
	{
		return false;
	}

	delay = this->getSatDelay();
	return true;
}
