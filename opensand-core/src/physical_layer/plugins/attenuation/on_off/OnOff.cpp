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
 * @file OnOff.cpp
 * @brief OnOff
 * @author Fatima LAHMOUAD <fatima.lahmouad@etu.enseeiht.fr>
 * @author Santiago PENA <santiago.penaluque@cnes.fr>
 * @author Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.fr>
 */


#include "OnOff.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>


OnOff::OnOff():
		AttenuationModelPlugin(),
		duration_counter(0)
{
}


OnOff::~OnOff()
{
}


void OnOff::generateConfiguration(const std::string &parent_path,
                                  const std::string &param_id,
                                  const std::string &plugin_name)
{
	auto Conf = OpenSandModelConf::Get();
	auto types = Conf->getModelTypesDefinition();

	auto attenuation = Conf->getComponentByPath(parent_path);
	if (attenuation == nullptr)
	{
		return;
	}
	auto attenuation_type = attenuation->getParameter(param_id);
	if (attenuation_type == nullptr)
	{
		return;
	}

	auto attenuation_on = attenuation->addParameter("onoff_attenuation_on",
	                                                "Attenuation On Duration",
	                                                types->getType("int"));
	attenuation_on->setUnit("refresh period");
	Conf->setProfileReference(attenuation_on, attenuation_type, plugin_name);
	auto attenuation_off = attenuation->addParameter("onoff_attenuation_off",
	                                                 "Attenuation Off Duration",
	                                                 types->getType("int"));
	attenuation_off->setUnit("refresh period");
	Conf->setProfileReference(attenuation_off, attenuation_type, plugin_name);
	auto attenuation_value = attenuation->addParameter("onoff_attenuation_amplitude",
													   "Attenuation On/Off Amplitude",
	                                                   types->getType("double"));
	attenuation_value->setUnit("dB");
	Conf->setProfileReference(attenuation_value, attenuation_type, plugin_name);
}


bool OnOff::init(time_ms_t refresh_period_ms, std::string link_path)
{
	this->refresh_period = refresh_period_ms;

	auto attenuation = OpenSandModelConf::Get()->getProfileData(link_path);
	auto parameter_on = attenuation->getParameter("onoff_attenuation_on");
	if(!OpenSandModelConf::extractParameterData(parameter_on, this->on_duration))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "On/Off attenuation %s: cannot get ON duration",
		    link_path.c_str());
		return false;
	}

	auto parameter_off = attenuation->getParameter("onoff_attenuation_off");
	if(!OpenSandModelConf::extractParameterData(parameter_off, this->off_duration))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "On/Off attenuation %s: cannot get OFF duration",
		    link_path.c_str());
		return false;
	}

	auto parameter_amplitude = attenuation->getParameter("onoff_attenuation_amplitude");
	if(!OpenSandModelConf::extractParameterData(parameter_amplitude, this->amplitude))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "On/Off attenuation %s: cannot get amplitude",
		    link_path.c_str());
		return false;
	}

	return true;
}


bool OnOff::updateAttenuationModel()
{
	this->duration_counter = (this->duration_counter + 1) %
	                         (this->on_duration + this->off_duration);

	LOG(this->log_attenuation, LEVEL_INFO,
	    "Attenuation model counter %d\n", this->duration_counter);
	if(this->duration_counter < this->off_duration)
	{
		this->setAttenuation(0);
	}
	else
	{
		this->setAttenuation(this->amplitude);
	}

	LOG(this->log_attenuation, LEVEL_INFO,
	    "On/Off Attenuation %.2f dB\n", this->getAttenuation());
	return true;
}
