/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
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

#include <opensand_conf/uti_debug.h>

#include "Sof.h"


Sof::Sof(uint16_t sf_nbr):
	OpenSandFrame<T_DVB_SOF>(sizeof(T_DVB_SOF))
{
	this->setMessageType(MSG_TYPE_SOF);
	this->setLength(sizeof(T_DVB_SOF));
	this->frame->sf_nbr = htons(sf_nbr);
}

Sof::Sof(unsigned char *frame, size_t length):
	OpenSandFrame<T_DVB_SOF>(frame, length)
{
	if(this->getMessageType() != MSG_TYPE_SOF)
	{
		UTI_ERROR("Frame is not a sof\n");
	}
}

Sof::~Sof()
{
}

uint16_t Sof::getSuperFrameNumber(void) const
{
	return ntohs(this->frame->sf_nbr);
}


