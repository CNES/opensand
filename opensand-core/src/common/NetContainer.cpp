/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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
 * @file NetContainer.cpp
 * @brief Network data container
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#include "NetContainer.h"


NetContainer::NetContainer(const unsigned char *data, size_t length):
	data(),
	name("unknown"),
	header_length(0),
	trailer_length(0),
	spot(255)
{
	this->data.append(data, length);
}

NetContainer::NetContainer(const Data &data, size_t length):
	data(data, 0, length),
	name("unknown"),
	header_length(0),
	trailer_length(0),
	spot(255)
{
}

NetContainer::NetContainer(const Data &data):
	data(data),
	name("unknown"),
	header_length(0),
	trailer_length(0),
	spot(255)
{
}

NetContainer::NetContainer():
	data(),
	name("unknown"),
	header_length(0),
	trailer_length(0),
	spot(255)
{
}

NetContainer::~NetContainer()
{
}


string NetContainer::getName() const
{
	return this->name;
}

Data NetContainer::getData() const
{
	return this->data;
}

Data NetContainer::getData(size_t pos) const
{
	return this->data.substr(pos, this->getTotalLength() - pos);
}


Data NetContainer::getPayload() const
{
	return this->data.substr(this->header_length,
	                         this->getPayloadLength());
}

Data NetContainer::getPayload(size_t pos) const
{
	return this->data.substr(this->header_length + pos,
	                         this->getPayloadLength());
}


size_t NetContainer::getPayloadLength() const
{
	return (this->getTotalLength() -
	        this->header_length -
	        this->trailer_length);
}

size_t NetContainer::getTotalLength() const
{
	return this->data.length();
}

size_t NetContainer::getHeaderLength() const
{
	return this->header_length;
}

void NetContainer::setSpot(spot_id_t spot_id)
{
	this->spot = spot_id;
}

spot_id_t NetContainer::getSpot() const
{
	return this->spot;
}

