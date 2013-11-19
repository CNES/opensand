/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
                     unsigned long simu_column_num,
                     unsigned int fwd_modcod_id,
                     unsigned int ret_modcod_id)
{
	this->id = id;
	this->simu_column_num = simu_column_num;
	this->current_fwd_modcod_id = fwd_modcod_id;
	this->previous_fwd_modcod_id = this->current_fwd_modcod_id;
	this->current_ret_modcod_id = ret_modcod_id;

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
	return this->simu_column_num;
}


unsigned int StFmtSimu::getCurrentFwdModcodId() const
{
	return this->current_fwd_modcod_id;
}

void StFmtSimu::updateFwdModcodId(unsigned int new_id, bool advertise)
{
	this->previous_fwd_modcod_id = this->current_fwd_modcod_id;
	this->current_fwd_modcod_id = new_id;

	// mark the down/forward MODCOD as not advertised yet if the MODCOD changed
	if(this->current_fwd_modcod_id != this->previous_fwd_modcod_id && advertise)
	{
		this->is_current_modcod_advertised = false;
	}
}


unsigned int StFmtSimu::getPreviousFwdModcodId() const
{
	if(this->previous_fwd_modcod_id > this->current_fwd_modcod_id)
	{
		// will be decoded
		return this->current_ret_modcod_id;
	}
	return this->previous_fwd_modcod_id;
}


bool StFmtSimu::isCurrentFwdModcodAdvertised() const
{
	return this->is_current_modcod_advertised;
}


void StFmtSimu::setFwdModcodAdvertised(void)
{
	this->is_current_modcod_advertised = true;
}


unsigned int StFmtSimu::getCurrentRetModcodId() const
{
	return this->current_ret_modcod_id;
}

void StFmtSimu::updateRetModcodId(unsigned int new_id)
{
	this->current_ret_modcod_id = new_id;
}
