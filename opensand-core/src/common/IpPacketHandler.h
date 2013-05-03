/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 CNES
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
 * @file IpPacketHandler.h
 * @brief Handler for IP packet
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#ifndef IP_PACKET_HANDLER_H
#define IP_PACKET_HANDLER_H

// debug
#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP

#include "EncapPlugin.h"
#include "IpPacket.h"
#include "Ipv4Packet.h"
#include "Ipv6Packet.h"

#include <opensand_conf/uti_debug.h>

#include <cassert>


/**
 * @class EncapPacketHandler
 * @brief Functions to handle the encapsulated packets
 */
class IpPacketHandler:
	public EncapPlugin::EncapPacketHandler
{

  public:

	IpPacketHandler(EncapPlugin &plugin):
		EncapPlugin::EncapPacketHandler(plugin)
	{};


	size_t getFixedLength() const {return 0;};
	NetPacket *build(unsigned char *data, size_t data_length,
	                 uint8_t qos, uint8_t src_tal_id, uint8_t dst_tal_id)
	{
		if(IpPacket::version(data, data_length) == 4)
		{
			Ipv4Packet *packet;
			packet = new Ipv4Packet(data, data_length);
			packet->setQos(qos);
			packet->setSrcTalId(src_tal_id);
			packet->setDstTalId(dst_tal_id);
			return packet;
		}
		else if(IpPacket::version(data, data_length) == 6)
		{
			Ipv6Packet *packet;
			packet = new Ipv6Packet(data, data_length);
			packet->setQos(qos);
			packet->setSrcTalId(src_tal_id);
			packet->setDstTalId(dst_tal_id);
			return packet;
		}
		else
		{
			UTI_ERROR("[IpPacketHandler::build] cannot get IP version\n");
			return NULL;
		}
	};

	size_t getLength(const unsigned char *data) const
	{
		return 0;
	};

	size_t getMinLength() const
	{
		assert(0);
	};

	bool getChunk(NetPacket *packet, size_t remaining_length,
	              NetPacket **data, NetPacket **remaining_data)
	{
		assert(0);
	};

	uint16_t getEtherType() const {return NET_PROTO_ERROR;};
	std::string const getName() {return "IP";};
};


#endif
