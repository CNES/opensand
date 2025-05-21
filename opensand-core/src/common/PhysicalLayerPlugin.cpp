/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 CNES
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
 * @file PhysicalLayerPlugin.cpp
 * @brief Plugins for Physical Layer Minimal conditions,
 *        Error insertion, Attenuation and SatDelay models
 */

#include "PhysicalLayerPlugin.h"

#include <opensand_output/Output.h>
#include <opensand_rt/Types.h>


AttenuationModelPlugin::AttenuationModelPlugin():
		OpenSandPlugin()
{
	this->log_init = Output::Get()->registerLog(LEVEL_WARNING, "Physical_Layer.init");
	this->log_attenuation = Output::Get()->registerLog(LEVEL_WARNING, "Physical_Layer.Attenuation");
}

AttenuationModelPlugin::~AttenuationModelPlugin()
{
}

void AttenuationModelPlugin::setAttenuation(double attenuation)
{
	this->attenuation = attenuation;
}

double AttenuationModelPlugin::getAttenuation() const
{
	return this->attenuation;
}


MinimalConditionPlugin::MinimalConditionPlugin():
		OpenSandPlugin()
{
	this->log_init = Output::Get()->registerLog(LEVEL_WARNING, "Physical_Layer.Init");
	this->log_minimal = Output::Get()->registerLog(LEVEL_WARNING, "Physical_Layer.MinimalCondition");
}

MinimalConditionPlugin::~MinimalConditionPlugin()
{
}

double MinimalConditionPlugin::getMinimalCN() const
{
	return this->minimal_cn;
}


ErrorInsertionPlugin::ErrorInsertionPlugin():
		OpenSandPlugin()
{
	this->log_init = Output::Get()->registerLog(LEVEL_WARNING, "Physical_Layer.Init");
	this->log_error = Output::Get()->registerLog(LEVEL_WARNING, "Physical_Layer.ErrorInsertion");
}

ErrorInsertionPlugin::~ErrorInsertionPlugin()
{
}


SatDelayPlugin::SatDelayPlugin():
		OpenSandPlugin(),
		delay(time_ms_t::zero()),
		refresh_period(1000),
		delay_mutex()
{
	this->log_init = Output::Get()->registerLog(LEVEL_WARNING, "SatDelay.init");
	this->log_delay = Output::Get()->registerLog(LEVEL_WARNING, "SatDelay.Delay");
}

SatDelayPlugin::~SatDelayPlugin()
{
}

time_ms_t SatDelayPlugin::getSatDelay() const
{
	std::lock_guard<std::mutex> lock{this->delay_mutex};
	return this->delay;
}

void SatDelayPlugin::setSatDelay(time_ms_t delay)
{
	std::lock_guard<std::mutex> lock{this->delay_mutex};
	this->delay = delay;
}

time_ms_t SatDelayPlugin::getRefreshPeriod() const
{
	return this->refresh_period;
}
