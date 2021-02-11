/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
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
 * @file RleIdentifier.cpp
 * @brief RLE identifier (unique index given by the association of
 *        the Tal Id and Mac Id)
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include "RleIdentifier.h"


RleIdentifier::RleIdentifier(uint8_t src_tal_id, uint8_t dst_tal_id):
	src_tal_id(src_tal_id),
	dst_tal_id(dst_tal_id)
{
}


RleIdentifier::~RleIdentifier()
{
}


uint8_t RleIdentifier::getSrcTalId() const
{
	return this->src_tal_id;
}


uint8_t RleIdentifier::getDstTalId() const
{
	return this->dst_tal_id;
}


bool ltRleIdentifier::operator() (RleIdentifier *ai1, RleIdentifier *ai2) const
{
	const auto talId1 = ai1->getSrcTalId();
	const auto talId2 = ai2->getSrcTalId();
	return talId1 == talId2 ? ai1->getDstTalId() < ai2->getDstTalId() : talId1 < talId2;
}
