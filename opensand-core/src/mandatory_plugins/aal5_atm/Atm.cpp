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
 * @file Atm.cpp
 * @brief ATM encapsulation plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#include "Atm.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include <opensand_conf/uti_debug.h>
#include <vector>
#include <map>


Atm::Atm():
	EncapPlugin(NET_PROTO_ATM)
{
	this->upper[TRANSPARENT].push_back("ROHC");
	this->upper[TRANSPARENT].push_back("PHS");
	this->upper[TRANSPARENT].push_back("IP");
	this->upper[TRANSPARENT].push_back("Ethernet");
	this->upper[REGENERATIVE].push_back("ROHC");
	this->upper[REGENERATIVE].push_back("PHS");
	this->upper[REGENERATIVE].push_back("IP");
	this->upper[REGENERATIVE].push_back("Ethernet");
}


Atm::Context::Context(EncapPlugin &plugin):
	EncapPlugin::EncapContext(plugin), contexts()
{
}

Atm::Context::~Context()
{
	std::map < AtmIdentifier *, Data *, ltAtmIdentifier >::iterator it;

	for(it = this->contexts.begin(); it != this->contexts.end(); it++)
	{
		if((*it).first != NULL)
		{
			delete (*it).first;
		}
		if((*it).second != NULL)
		{
			delete (*it).second;
		}
	}
}

NetBurst *Atm::Context::encapsulate(NetBurst *burst,
                                    std::map<long, int> &UNUSED(time_contexts))
{
	const char *FUNCNAME = "[Atm::Context::encapsulate]";
	NetBurst *atm_cells = NULL;
	NetBurst::iterator packet;

	// create an empty burst of ATM cells
	atm_cells = new NetBurst();
	if(atm_cells == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of ATM cells\n",
		          FUNCNAME);
		delete burst;
		return NULL;
	}

	for(packet = burst->begin(); packet != burst->end(); packet++)
	{
		Aal5Packet *aal5_packet;

		aal5_packet = this->encapAal5(*packet);
		if(!aal5_packet)
		{
			UTI_ERROR("%s AAL5 encapsulation failed, drop packet\n", FUNCNAME);
			continue;
		}
		if(!this->encapAtm(aal5_packet, atm_cells))
		{
			UTI_ERROR("%s ATM encapsulation failed, drop packet\n", FUNCNAME);
			continue;
		}
	}

	// delete the burst and all packets in it
	delete burst;
	return atm_cells;
}


NetBurst *Atm::Context::deencapsulate(NetBurst *burst)
{
	const char *FUNCNAME = "[Atm::Context::deencapsulate]";
	NetBurst *net_packets;

	NetBurst::iterator packet;

	// create an empty burst of network packets
	net_packets = new NetBurst();
	if(net_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of network packets\n",
		          FUNCNAME);
		delete burst;
		return false;
	}

	for(packet = burst->begin(); packet != burst->end(); packet++)
	{
		NetBurst *aal5_packets;
		uint8_t dst_tal_id;

		// packet must be valid
		if(*packet == NULL)
		{
			UTI_ERROR("%s encapsulation packet is not valid, drop the packet\n",
			          FUNCNAME);
			continue;
		}

		// packet must be an AAL5/ATM packet
		if((*packet)->getType() != this->getEtherType())
		{
			UTI_ERROR("%s encapsulation packet is not an AAL5/ATM packet "
			          "(type = 0x%04x), drop the packet\n",
			          FUNCNAME, (*packet)->getType());
			continue;
		}

		// Filter if packet is for this ST
		dst_tal_id = (*packet)->getDstTalId();
		if((dst_tal_id != this->dst_tal_id)
			&& (dst_tal_id != BROADCAST_TAL_ID))
		{
			UTI_DEBUG("%s encapsulation packet is for ST#%u. Drop\n",
			          FUNCNAME, dst_tal_id);
			continue;
		}

		aal5_packets = this->deencapAtm(*packet);
		if(aal5_packets == NULL)
		{
			UTI_ERROR("%s ATM desencapsulation failed, drop packet\n", FUNCNAME);
			continue;
		}
		if(!this->deencapAal5(aal5_packets, net_packets))
		{
			UTI_ERROR("%s cannot create a burst of packets, drop packet\n", FUNCNAME);
			continue;
		}
	}

	// delete the burst and all packets in it
	delete burst;
	return net_packets;
}

bool Atm::Context::encapAtm(Aal5Packet *packet,
                            NetBurst *atm_cells)
{
	const char *FUNCNAME = "Atm::Context::encapAtm";
	AtmCell *atm_cell;
	NetPacket *atm;
	int nb_atm_cells, i;
	uint8_t vpi;
	uint16_t vci;
	// keep the destination spot
	uint16_t dest_spot = packet->getDstSpot();
	// keep the source/destination tal_id
	uint8_t src_tal_id = packet->getSrcTalId();
	uint8_t dst_tal_id = packet->getDstTalId();
	// keep the QoS
	uint8_t qos = packet->getQos();

	// packet must be an AAL5 packet
	if(packet->getType() != NET_PROTO_AAL5)
	{
		UTI_ERROR("%s encapsulation packet is not an AAL5 packet, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	UTI_DEBUG_L3("%s talID of packet to encapsulate: %u \n", FUNCNAME,
	             packet->getDstTalId());

	nb_atm_cells = packet->nbAtmCells();
	for(i = 0; i < nb_atm_cells; i++)
	{
		if((src_tal_id & 0x1F) != src_tal_id)
		{
			UTI_ERROR("Be careful, you have set a source TAL ID greater than 0x1f,"
			           " this can not stand in the VCI/VPI field of ATM cells!!!\n");
		}
		if((dst_tal_id & 0x1F) != dst_tal_id)
		{
			UTI_ERROR("Be careful, you have set a destination TAL ID greater than 0x1f,"
			           " this can not stand in the VCI/VPC field of ATM cells!!!\n");
		}
		if((qos & 0x07) != qos)
		{
			UTI_ERROR("Be careful, you have set a QoS priority greater than 0x07,"
			           " this can not stand in the last 3 bits of VCI field of"
			           "  ATM cells!!!\n");
		}

		vci = AtmCell::getVci(packet);
		vpi = AtmCell::getVpi(packet);

		atm_cell = AtmCell::create(i, vpi, vci, 0x40, 0,
		                          (i == (nb_atm_cells - 1)),
		                          packet->atmCell(i));
		if(atm_cell == NULL)
		{
			UTI_ERROR("%s cannot allocate memory for one ATM cell, drop it\n",
			          FUNCNAME);
			continue;
		}
		atm = this->createPacket(atm_cell->getData(),
		                         atm_cell->getTotalLength(),
		                         qos, src_tal_id, dst_tal_id);
		delete atm_cell;
		if(atm == NULL)
		{
			UTI_ERROR("%s cannot create one ATM cell, drop it\n",
			          FUNCNAME);
			continue;
		}

		UTI_DEBUG("%s one ATM cell created with QoS %d\n",
		          FUNCNAME, atm->getQos());

		// set the desintation spot ID
		atm->setDstSpot(dest_spot);
		// add the ATM cell to the list
		atm_cells->add(atm);
	}

	delete packet;
	return true;

drop:
	delete packet;
	return false;
}

// TODO for here and other encap/deencapmethods : handle endianess !!
NetBurst *Atm::Context::deencapAtm(NetPacket *packet)
{
	const char *FUNCNAME = "[Atm::Context::deencapAtm]";
	NetBurst *aal5_packets;
	AtmCell *atm_cell;
	uint8_t vpi;
	uint16_t vci;
	AtmIdentifier *atm_id;
	std::map < AtmIdentifier *, Data *,
	           ltAtmIdentifier >::iterator context_it;
	Data *context;
	// keep the destination spot
	uint16_t dest_spot = packet->getDstSpot();
	// keep the source/destination tal_id
	uint8_t src_tal_id = packet->getSrcTalId();
	uint8_t dst_tal_id = packet->getDstTalId();
	// keep the QoS
	uint8_t qos = packet->getQos();

	UTI_DEBUG_L3("%s talID of received packet: %u \n",
	             FUNCNAME, packet->getDstTalId());

	// cast from a generic packet to an ATM cell
	atm_cell = new AtmCell(packet->getData());
	if(atm_cell == NULL)
	{
		UTI_ERROR("%s cannot create AtmCell from NetPacket\n", FUNCNAME);
		goto error;
	}

	// get the VPI and VCI numbers for the ATM cell to desencapsulate
	vpi = atm_cell->getVpi();
	vci = atm_cell->getVci();
	UTI_DEBUG("%s ATM packet belongs to the encapsulation context "
	          "identified by VPI = %d and VCI = %d\n", FUNCNAME, vpi, vci);

	// find the desencapsulation context for the ATM cell
	atm_id = new AtmIdentifier(vpi, vci);
	context_it = this->contexts.find(atm_id);
	if(context_it == this->contexts.end())
	{
		UTI_DEBUG("%s desencapsulation context does not exist yet\n", FUNCNAME);

		Data *new_context = new Data();
		std::pair < std::map < AtmIdentifier *, Data *,
		            ltAtmIdentifier >::iterator, bool > infos;
		infos = this->contexts.insert(make_pair(atm_id, new_context));

		if(!infos.second)
		{
			UTI_ERROR("%s cannot create a new desencapsulation context, "
			          "drop the packet\n", FUNCNAME);
			delete new_context;
			delete atm_id;
			goto drop;
		}

		UTI_INFO("%s new desencapsulation context created (VPI = %d, "
		         "VCI = %d)\n", FUNCNAME, vpi, vci);
		context = (*(infos.first)).second;
	}
	else
	{
		UTI_DEBUG_L3("%s desencapsulation context already exists\n", FUNCNAME);
		context = (*context_it).second;
		delete atm_id;
	}

	UTI_DEBUG("%s desencapsulation context contains %zu bytes of data\n",
	          FUNCNAME, context->length());

	// create an empty burst of AAL5 packets
	aal5_packets = new NetBurst();
	if(aal5_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of AAL5 packets\n",
		          FUNCNAME);
		goto drop;
	}

	// desencapsulate the ATM payload data

	if(!atm_cell->isLastCell())
	{
		UTI_DEBUG("%s ATM cell is not the last one of AAL5 packet, store "
		          "payload data in the desencapsulation context for "
		          "next ATM cell\n", FUNCNAME);

		context->append(atm_cell->getPayload());
	}
	else
	{
		Aal5Packet *aal5_packet;

		UTI_DEBUG("%s ATM cell is the last one of AAL5 packet, "
		          "extract the AAL5 packet from ATM payloads\n", FUNCNAME);

		context->append(atm_cell->getPayload());

		// create an AAL5 packet with ATM payloads
		aal5_packet = new Aal5Packet(*context);
		if(aal5_packet == NULL)
		{
			UTI_ERROR("%s cannot create an AAL5 packet, drop all of the "
			          "ATM cells in the desencapsulation context\n", FUNCNAME);
			goto clear;
		}
		// check AAL5 packet validity
		if(!aal5_packet->isValid())
		{
			UTI_ERROR("%s AAL5 packet is not valid, drop all of the "
			          "ATM cells in the desencapsulation context\n", FUNCNAME);
			delete aal5_packet;
			goto clear;
		}

		// set the destination spot ID
		aal5_packet->setDstSpot(dest_spot);
		// Set the source/destination tal Id
		aal5_packet->setSrcTalId(src_tal_id);
		aal5_packet->setDstTalId(dst_tal_id);
		// set the Qos
		aal5_packet->setQos(qos);

		// add the AAL5 packet to the list
		aal5_packets->add(aal5_packet);
		UTI_DEBUG("%s AAL5 packet added to the burst\n", FUNCNAME);

		// clear data stored in context
		context->clear();
	}

	UTI_DEBUG("%s ATM cell is now desencapsulated "
	          "(context data = %zu bytes)\n",
	          FUNCNAME, context->length());

	delete atm_cell;
	return aal5_packets;

clear:
	// clear data stored in context
	context->clear();
	// delete the burst of AAL5 packets
	delete aal5_packets;
drop:
	delete atm_cell;
error:
	return NULL;
}

Aal5Packet *Atm::Context::encapAal5(NetPacket *packet)
{
	const char *FUNCNAME = "[Atm::Context::encapAal5]";
	Aal5Packet *aal5_packet;

	UTI_DEBUG("received a packet with type 0x%.4x\n", packet->getType());

	// build an AAL5 packet with a network packet as payload
	aal5_packet = Aal5Packet::createFromPayload(packet->getData());
	if(aal5_packet == NULL)
	{
		UTI_ERROR("%s cannot create an AAL5 packet, "
		          "drop the network packet\n", FUNCNAME);
		goto drop;
	}
	aal5_packet->setDstTalId(packet->getDstTalId());
	aal5_packet->setSrcTalId(packet->getSrcTalId());
	aal5_packet->setQos(packet->getQos());

	// check AAL5 packet validity
	if(!aal5_packet->isValid())
	{
		UTI_ERROR("%s AAL5 packet is not valid, "
		          "drop the network packet\n", FUNCNAME);
		goto clean;
	}

	UTI_DEBUG("%s AAL5 packet is valid (QoS %d)\n",
	          FUNCNAME, aal5_packet->getQos());


	return aal5_packet;

clean:
	delete aal5_packet;
drop:
	return NULL;
}

bool Atm::Context::deencapAal5(NetBurst *aal5_packets,
                               NetBurst *net_packets)
{
	const char *FUNCNAME = "[Atm::Context::deencapAal5]";
	NetPacket *packet;
	Aal5Packet *aal5_packet;

	NetBurst::iterator it;

	for(it = aal5_packets->begin(); it != aal5_packets->end(); it++)
	{
		uint16_t dest_spot;
		uint8_t src_tal_id, dst_tal_id;
		uint8_t qos;

		// cast from a generic packet to an AAL5 packet
		aal5_packet = dynamic_cast<Aal5Packet *>(*it);
		if(aal5_packet == NULL)
		{
			UTI_ERROR("%s bad cast from NetPacket to Aal5Packet!\n",
			          FUNCNAME);
			continue;
		}

		// keep the destination spot
		dest_spot = aal5_packet->getDstSpot();
		// keep the source/destination tal_id
		src_tal_id = aal5_packet->getSrcTalId();
		dst_tal_id = aal5_packet->getDstTalId();
		qos = aal5_packet->getQos();

		packet = this->current_upper->build(
				aal5_packet->getPayload(),
				aal5_packet->getPayloadLength(),
				qos, src_tal_id, dst_tal_id);
		if(packet == NULL)
		{
			UTI_ERROR("%s cannot build a %s packet, drop the AAL5 packet\n",
			          FUNCNAME, this->current_upper->getName().c_str());
			continue;
		}

		// set the destination spot ID
		packet->setDstSpot(dest_spot);

		// add the IP packet to the list
		net_packets->add(packet);

		UTI_DEBUG("%s %s packet added to the burst (proto %u)\n", FUNCNAME,
		          packet->getName().c_str(), packet->getType());

	}
	// delete the burst and all the packets in it
	delete aal5_packets;

	return true;
}


NetPacket *Atm::PacketHandler::build(const Data &data,
                                     size_t data_length,
                                     uint8_t UNUSED(_qos),
                                     uint8_t UNUSED(_src_tal_id),
                                     uint8_t UNUSED(_dst_tal_id)) const
{
	const char *FUNCNAME = "[Atm::PacketHandler::build]";
	uint8_t qos;
	uint8_t src_tal_id, dst_tal_id;

	if(data_length != this->getFixedLength())
	{
		UTI_ERROR("%s bad data length (%zu) for ATM cell\n",
		          FUNCNAME, data_length);
		return NULL;
	}

	AtmCell atm_cell(data, data_length);
	qos = atm_cell.getQos();
	src_tal_id = atm_cell.getSrcTalId();
	dst_tal_id = atm_cell.getDstTalId();

	return new NetPacket(data, data_length,
	                     this->getName(), this->getEtherType(),
	                     qos, src_tal_id, dst_tal_id, 5);
}

bool Atm::PacketHandler::getChunk(NetPacket *packet, size_t remaining_length,
                                  NetPacket **data, NetPacket **remaining_data) const
{
	*data = NULL;
	*remaining_data = NULL;
	if(remaining_length < this->getFixedLength())
	{
		*remaining_data = packet;
	}
	else
	{
		*data = packet;
	}

	return true;
}

Atm::PacketHandler::PacketHandler(EncapPlugin &plugin):
	EncapPlugin::EncapPacketHandler(plugin)
{
}


bool Atm::PacketHandler::getSrc(const Data &data, tal_id_t &tal_id) const
{
	AtmCell atm_cell(data, this->getFixedLength());
	if(!atm_cell.isValid())
	{
		return false;
	}
	tal_id = atm_cell.getSrcTalId();
	return true;
}
