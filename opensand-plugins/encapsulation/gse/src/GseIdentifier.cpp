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
 * @file GseIdentifier.cpp
 * @brief GSE identifier (unique index given by the association of
 *        the Tal Id and Mac Id and QoS of the packets)
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */


#include "GseIdentifier.h"


GseIdentifier::GseIdentifier(uint8_t src_tal_id,
                             uint8_t dst_tal_id,
                             uint8_t qos)
{
	this->src_tal_id = src_tal_id;
	this->dst_tal_id = dst_tal_id;
	this->qos = qos;
}


GseIdentifier::~GseIdentifier()
{
}


uint8_t GseIdentifier::getSrcTalId() const
{
	return this->src_tal_id;
}


uint8_t GseIdentifier::getDstTalId() const
{
	return this->dst_tal_id;
}


uint8_t GseIdentifier::getQos() const
{
	return this->qos;
}


bool ltGseIdentifier::operator() (GseIdentifier *ai1, GseIdentifier *ai2) const
{
	const auto srcTalId1 = ai1->getSrcTalId();
	const auto srcTalId2 = ai2->getSrcTalId();

	if (srcTalId1 == srcTalId2)
	{
		const auto dstTalId1 = ai1->getDstTalId();
		const auto dstTalId2 = ai2->getDstTalId();

		return dstTalId1 == dstTalId2 ? ai1->getQos() < ai2->getQos() : dstTalId1 < dstTalId2;
	}

	return srcTalId1 < srcTalId2;
}
