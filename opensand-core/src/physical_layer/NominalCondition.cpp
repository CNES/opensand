/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
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
 * @file NominalCondition.cpp
 * @brief NominalCondition
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */

#include "NominalCondition.h"

#define DBG_PREFIX
#define DBG_PACKAGE PKG_PHY_LAYER
#include <opensand_conf/uti_debug.h>

NominalCondition::NominalCondition(string nominal_condition_mode)
{
	this->nominal_condition_mode = nominal_condition_mode;
}

NominalCondition::~NominalCondition()
{
}

string NominalCondition::getNominalConditionMode()
{
	return this->nominal_condition_mode;
}

void NominalCondition::setNominalConditionMode(string nominal_condition_mode)
{
	this->nominal_condition_mode = nominal_condition_mode;
}

