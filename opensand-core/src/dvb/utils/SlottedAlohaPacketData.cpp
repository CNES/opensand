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
 * @file SlottedAlohaPacketData.cpp
 * @brief The Slotted Aloha data packets
 *
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
 */

#include "SlottedAlohaPacketData.h"

#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <algorithm>

// TODO idea for everywhere, create a template class for endianess handling
//      that abstract the type (uint8_t, uint16_t, uint32_t, uint64_t, ...)
//      this avoid changing everything when changing a type

// TODO qos is maybe not useful in header
SlottedAlohaPacketData::SlottedAlohaPacketData(const Rt::Data &data,
                                               saloha_pdu_id_t id,
                                               uint16_t ts,
                                               uint16_t seq,
                                               uint16_t pdu_nb,
                                               uint16_t nb_replicas,
                                               time_sf_t timeout_saf):
	SlottedAlohaPacket(data)
{
	saloha_data_hdr_t tmp_head;
	this->name = "Slotted Aloha data";
	this->header_length = sizeof(saloha_data_hdr_t);
	this->timeout_saf = timeout_saf;
	this->nb_retransmissions = 0;

	tmp_head.id = htonl(id);
	tmp_head.ts = htons(ts);
	tmp_head.seq = htons(seq);
	tmp_head.pdu_nb = htons(pdu_nb);
	tmp_head.nb_replicas = 0;
	this->data.insert(0, reinterpret_cast<unsigned char *>(&tmp_head), this->header_length);

	this->setReplicas(nullptr, nb_replicas);
	this->header_length = sizeof(saloha_data_hdr_t) + nb_replicas * sizeof(uint16_t);

	reinterpret_cast<saloha_data_hdr_t *>(this->data.data())->total_length = htons(this->data.length());
}

SlottedAlohaPacketData::SlottedAlohaPacketData(const Rt::Data &data, size_t length):
	SlottedAlohaPacket(data, length)
{
	this->name = "Slotted Aloha data";
	this->header_length = sizeof(saloha_data_hdr_t);
}

SlottedAlohaPacketData::~SlottedAlohaPacketData()
{
}

const saloha_data_hdr_t *SlottedAlohaPacketData::header() const
{
	return reinterpret_cast<const saloha_data_hdr_t *>(this->data.data());
}

saloha_pdu_id_t SlottedAlohaPacketData::getId() const
{
	// if uint64_t
	//return be64toh(header()->id);
	// if uint32_t
	return ntohl(header()->id);
}

uint16_t SlottedAlohaPacketData::getTs() const
{
	return ntohs(header()->ts);
}

uint16_t SlottedAlohaPacketData::getSeq() const
{
	return ntohs(header()->seq);
}

uint16_t SlottedAlohaPacketData::getPduNb() const
{
	return ntohs(header()->pdu_nb);
}

uint16_t SlottedAlohaPacketData::getTimeout() const
{
	return this->timeout_saf;
}

uint16_t SlottedAlohaPacketData::getNbRetransmissions() const
{
	return this->nb_retransmissions;
}

uint16_t SlottedAlohaPacketData::getNbReplicas() const
{
	return ntohs(header()->nb_replicas);
}

size_t SlottedAlohaPacketData::getReplicasLength() const
{
	return this->getNbReplicas() * sizeof(uint16_t);
}

uint16_t SlottedAlohaPacketData::getReplica(uint16_t pos) const
{
	if(this->getNbReplicas() < pos)
	{
		return 0;
	}
	return ntohs(header()->replicas[pos]);
}

uint8_t SlottedAlohaPacketData::getQos() const
{
	return header()->qos;
}

void SlottedAlohaPacketData::setTs(uint16_t ts)
{
	reinterpret_cast<saloha_data_hdr_t *>(this->data.data())->ts = htons(ts);
}

void SlottedAlohaPacketData::setReplicas(uint16_t *replicas, size_t nb_replicas)
{
	//Warning: a Slotted Aloha data packet (not ctrl signal) is composed
	//         as follows:
	//         <header><replicas><data> so we need to reserve some memory
	//         space to store array of replicas

	// first adjust replicas number in packet
	if(this->getNbReplicas() < nb_replicas)
	{
		size_t diff = (nb_replicas - this->getNbReplicas()) * sizeof(uint16_t);
		std::string zero(diff, 0);
		this->data.insert(this->header_length,
		                  (unsigned char *)zero.c_str(),
		                  diff);
	}
	if(this->getNbReplicas() > nb_replicas)
	{
		size_t diff = (this->getNbReplicas() - nb_replicas) * sizeof(uint16_t);
		this->data.erase(this->header_length,
		                 diff * sizeof(uint16_t));
	}

	saloha_data_hdr_t *header = reinterpret_cast<saloha_data_hdr_t *>(this->data.data());
	header->nb_replicas = htons(nb_replicas);
	if(!replicas)
	{
		return;
	}

	for(unsigned int i = 0; i < nb_replicas; i++)
	{
		header->replicas[i] = htons(replicas[i]);
	}
}

bool SlottedAlohaPacketData::isTimeout() const
{
	return this->timeout_saf <= 0;
}

void SlottedAlohaPacketData::setTimeout(time_sf_t timeout_saf)
{
	this->timeout_saf = timeout_saf;
}

void SlottedAlohaPacketData::decTimeout()
{
	this->timeout_saf = std::max((int)this->timeout_saf - 1, 0);
}

bool SlottedAlohaPacketData::canBeRetransmitted(uint16_t max_retransmissions) const
{
	return this->nb_retransmissions < max_retransmissions;
}

void SlottedAlohaPacketData::incNbRetransmissions()
{
	this->nb_retransmissions++;
}

size_t SlottedAlohaPacketData::getTotalLength() const
{
	return ntohs(header()->total_length);
}

size_t SlottedAlohaPacketData::getPayloadLength() const
{
	return this->getTotalLength() - sizeof(saloha_data_hdr_t) - this->getReplicasLength();
}

void SlottedAlohaPacketData::setQos(qos_t qos)
{
	NetPacket::setQos(qos);
	reinterpret_cast<saloha_data_hdr_t *>(this->data.data())->qos = qos;
}

Rt::Data SlottedAlohaPacketData::getPayload() const
{
	return this->data.substr(sizeof(saloha_data_hdr_t) +
	                         this->getReplicasLength(),
	                         this->getPayloadLength());
}

size_t SlottedAlohaPacketData::getPacketLength(const Rt::Data &data)
{
	const saloha_data_hdr_t *header = reinterpret_cast<const saloha_data_hdr_t*>(data.data());
	return ntohs(header->total_length);
}

saloha_id_t SlottedAlohaPacketData::getUniqueId(void) const
{
	/*
	std::ostringstream os;

	// need int cast else there is some problems
	os << (int)this->getId() << ':' << (int)this->getSeq() << ':'
	   << (int)this->getPduNb() << ':' << (int)this->getQos();
	*/
	Rt::ODataStream os;
	os << this->getId() << ':' << this->getSeq() << ':' << this->getPduNb() << ':' << this->getQos();
	return saloha_id_t(os.str());
}


