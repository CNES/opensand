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
 * @file Triangular.cpp
 * @brief Triangular
 * @author Fatima LAHMOUAD <fatima.lahmouad@etu.enseeiht.fr>
 * @author Santiago PENA <santiago.penaluque@cnes.fr>
 * @author Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.fr>
 */


#include "Triangular.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>


const std::string SLOPE = "triangle_attenuation_slope";
const std::string PERIOD = "triangle_attenuation_period";


Triangular::Triangular():
		AttenuationModelPlugin(),
		duration_counter(0)
{
}


Triangular::~Triangular()
{
}


void Triangular::generateConfiguration(const std::string &parent_path,
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

	auto attenuation_slope = attenuation->addParameter(SLOPE, "Attenuation Slope", types->getType("double"));
	attenuation_slope->setUnit("dB / refresh period");
	Conf->setProfileReference(attenuation_slope, attenuation_type, plugin_name);
	auto attenuation_period = attenuation->addParameter(PERIOD, "Attenuation Period", types->getType("int"));
	attenuation_period->setUnit("refresh period");
	Conf->setProfileReference(attenuation_period, attenuation_type, plugin_name);
}


bool Triangular::init(time_ms_t refresh_period_ms, std::string link_path)
{
	this->refresh_period = refresh_period_ms;

	auto attenuation = OpenSandModelConf::Get()->getProfileData(link_path);

	if(!OpenSandModelConf::extractParameterData(attenuation->getParameter(PERIOD), this->period))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Triangular attenuation %s: cannot get %s",
		    link_path.c_str(), PERIOD.c_str());
		return false;
	}

	if(!OpenSandModelConf::extractParameterData(attenuation->getParameter(SLOPE), this->slope))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Triangular attenuation %s: cannot get %s",
		    link_path.c_str(), SLOPE.c_str());
		return false;
	}

	return true;
}


bool Triangular::updateAttenuationModel()
{
	this->duration_counter = (this->duration_counter + 1) % this->period;
	double refresh_period = std::chrono::duration_cast<std::chrono::duration<double>>(this->refresh_period).count();
	double time = this->duration_counter * refresh_period;

	if(time < this->period / 2)
	{
		this->setAttenuation(time * this->slope);
	}
	else
	{
		double max = this->period * this->slope * refresh_period;
		this->setAttenuation(max - time * this->slope);
	}

	LOG(this->log_attenuation, LEVEL_INFO,
	    "On/Off Attenuation %.2f dB\n",
	    this->getAttenuation());

	return true;
}
