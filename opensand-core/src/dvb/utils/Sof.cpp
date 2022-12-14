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
 * @file    Sof.cpp
 * @brief   Represent a SOF
 * @author  Audric Schiltknecht / Viveris Technologies
 */

#include <opensand_output/Output.h>

#include "Sof.h"


Sof::Sof(time_sf_t sf_nbr):
	DvbFrameTpl<T_DVB_SOF>()
{
	this->setMessageType(EmulatedMessageType::Sof);
	this->setMessageLength(sizeof(T_DVB_SOF));
	this->frame()->sf_nbr = htons(sf_nbr);
}

Sof::~Sof()
{
}

time_sf_t Sof::getSuperFrameNumber(void) const
{
	return ntohs(this->frame()->sf_nbr);
}


