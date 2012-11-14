/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
 * Copyright © 2012 CNES
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
 * @file AttenuationModel.cpp
 * @brief AttenuationModel
 * @author Fatima LAHMOUAD <fatima.lahmouad@etu.enseeiht.fr>
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */


#define DBG_PREFIX
#define DBG_PACKAGE PKG_PHY_LAYER
#include <opensand_conf/uti_debug.h>


#include "AttenuationModel.h"

AttenuationModel::AttenuationModel(string attenuation_model_mode,
                                   int granularity)
{
	this->attenuation = 0;
	this->attenuation_model_mode = attenuation_model_mode;
	this->granularity = granularity;
	this->time_counter = 0;
}


AttenuationModel::~AttenuationModel()
{
}


string AttenuationModel::getAttenuationModelMode()
{
	return this->attenuation_model_mode;
}


void AttenuationModel::setAttenuationModelMode(string attenuation_model_mode)
{
	this->attenuation_model_mode = attenuation_model_mode;
}


double AttenuationModel::getAttenuation()
{
	return this->attenuation;
}


void AttenuationModel::setAttenuation(double attenuation)
{
	this->attenuation = attenuation;
}


int AttenuationModel::getTimeCounter()
{
	return this->time_counter;
}


void AttenuationModel::setTimeCounter(int time_counter)
{
	this->time_counter = time_counter;
}


int AttenuationModel::getGranularity()
{
	return this->granularity;
}


void AttenuationModel::setGranularity(int granularity)
{
	this->granularity = granularity;
}

