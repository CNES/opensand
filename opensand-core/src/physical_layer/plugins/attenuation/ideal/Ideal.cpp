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
 * @file Ideal.cpp
 * @brief Ideal
 * @author Santiago PENA <santiago.penaluque@cnes.fr>
 * @author Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.fr>
 */


#include "Ideal.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>


Ideal::Ideal():
		AttenuationModelPlugin()
{
}


Ideal::~Ideal()
{
}


void Ideal::generateConfiguration(const std::string &parent_path,
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

	auto attenuation_value = attenuation->addParameter("ideal_attenuation_value",
	                                                   "Attenuation Value",
	                                                   types->getType("double"));
	attenuation_value->setUnit("dB");
	Conf->setProfileReference(attenuation_value, attenuation_type, plugin_name);
}


bool Ideal::init(time_ms_t refresh_period_ms, std::string link_path)
{
	this->refresh_period = refresh_period_ms;

	auto attenuation = OpenSandModelConf::Get()->getProfileData(link_path);
	if(!OpenSandModelConf::extractParameterData(attenuation->getParameter("ideal_attenuation_value"), this->value))
	{
		LOG(this->log_init, LEVEL_ERROR, 
		    "Ideal attenuation %s: cannot get attenuation value",
		    link_path.c_str());
		return false;
	}

	return true;
}


bool Ideal::updateAttenuationModel()
{
	this->attenuation = this->value;
	LOG(this->log_init, LEVEL_INFO, 
	    "Constant attenuation: %.2f dB\n", this->getAttenuation());

	return true;
}
