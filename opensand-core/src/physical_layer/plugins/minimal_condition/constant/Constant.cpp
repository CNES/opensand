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
 * @file Constant.cpp
 * @brief Constant
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 * @author Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.fr>
 */


#include "Constant.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>

#include <sstream>

#define CONSTANT_SECTION "constant"
#define THRESHOLD        "threshold"
#define CONF_CST_FILENAME    "constant.conf"


std::string config_path = "";

Constant::Constant():
	MinimalConditionPlugin()
{
}

Constant::~Constant()
{  
}

void Constant::generateConfiguration(const std::string &parent_path,
	                                 const std::string &param_id,
	                                 const std::string &plugin_name)
{
	auto Conf = OpenSandModelConf::Get();
	auto types = Conf->getModelTypesDefinition();

	Constant::config_path = parent_path;
	auto minimal = Conf->getComponentByPath(parent_path);
	if(minimal == nullptr)
	{
		return;
	}
	auto minimal_type = minimal->getParameter(param_id);
	if(minimal_type == nullptr)
	{
		return;
	}

	auto minimal_cn = minimal->addParameter("threshold", "Threshold",
	                                        types->getType("double"),
	                                        "Threshold value for QEF communications");
	minimal_cn->setUnit("dB");
	Conf->setProfileReference(minimal_cn, minimal_type, plugin_name);
}

bool Constant::init(void)
{
	auto minimal = OpenSandModelConf::Get()->getProfileData(config_path);

	if(!OpenSandModelConf::extractParameterData(minimal->getParameter("threshold"), this->minimal_cn))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Constant minimal conditions: cannot get threshodl");
		return false;
	}

	return true;
}


bool Constant::updateThreshold(uint8_t UNUSED(modcod_id), uint8_t UNUSED(message_type))
{
	// Empty function, not necessary when
	// Constant Minimal conditions
	return true;
}

