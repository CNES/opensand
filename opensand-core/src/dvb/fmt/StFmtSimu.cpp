/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 * @file StFmtSimu.cpp
 * @brief The internal representation of a Satellite Terminal (ST)
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "StFmtSimu.h"


StFmtSimu::StFmtSimu(long id,
                     uint8_t modcod_id)
{
	this->id = id;
	this->current_modcod_id = modcod_id;
	this->previous_modcod_id = this->current_modcod_id;

	// do not advertise at startup because for physical layer scenario we do not
	// want advertisment
	this->is_current_modcod_advertised = true;
}


StFmtSimu::~StFmtSimu()
{
	// nothing particular to do
}


long StFmtSimu::getId() const
{
	return this->id;
}


unsigned long StFmtSimu::getSimuColumnNum() const
{
	// the column is the id
	return (unsigned long) this->id;
}


uint8_t StFmtSimu::getCurrentModcodId() const
{
	return this->current_modcod_id;
}

void StFmtSimu::updateModcodId(uint8_t new_id, bool advertise)
{
	this->previous_modcod_id = this->current_modcod_id;
	this->current_modcod_id = new_id;

	// mark the MODCOD as not advertised yet if the MODCOD changed (for down/forward)
	if(this->current_modcod_id != this->previous_modcod_id && advertise)
	{
		this->is_current_modcod_advertised = false;
	}
}


uint8_t StFmtSimu::getPreviousModcodId() const
{
	if(this->previous_modcod_id > this->current_modcod_id)
	{
		// will be decoded
		return this->current_modcod_id;
	}
	return this->previous_modcod_id;
}


bool StFmtSimu::isCurrentModcodAdvertised() const
{
	return this->is_current_modcod_advertised;
}


void StFmtSimu::setModcodAdvertised(void)
{
	this->is_current_modcod_advertised = true;
}



