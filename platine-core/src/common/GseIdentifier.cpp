/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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


GseIdentifier::GseIdentifier(long tal_id, unsigned long mac_id, int qos)
{
	this->_tal_id = tal_id;
	this->_mac_id = mac_id;
	this->_qos = qos;
}

GseIdentifier::~GseIdentifier()
{
}

long GseIdentifier::talId()
{
	return this->_tal_id;
}

unsigned long GseIdentifier::macId()
{
	return this->_mac_id;
}

int GseIdentifier::qos()
{
	return this->_qos;
}
