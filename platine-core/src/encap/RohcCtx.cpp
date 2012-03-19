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
 * @file RohcCtx.cpp
 * @brief ROHC encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "RohcCtx.h"

#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"

// NOTE:
// The ROHC library is not thread safe! The CRC tables are globals and the
// compresssor/decompressor/profiles/contexts are not protected against
// concurrent accesses.

RohcCtx::RohcCtx(): EncapCtx()
{
	const char *FUNCNAME = "[RohcCtx::RohcCtx]";

	// init the CRC tables of the ROHC library
	crc_init_table(crc_table_3, crc_get_polynom(CRC_TYPE_3));
	crc_init_table(crc_table_7, crc_get_polynom(CRC_TYPE_7));
	crc_init_table(crc_table_8, crc_get_polynom(CRC_TYPE_8));

	// create the ROHC compressor
	this->comp = rohc_alloc_compressor(15, 0, 0, 0);
	if(this->comp == NULL)
	{
		UTI_ERROR("%s cannot create ROHC compressor\n", FUNCNAME);
		goto error;
	}

	// activate the compression profiles
	rohc_activate_profile(this->comp, ROHC_PROFILE_UNCOMPRESSED);
	rohc_activate_profile(this->comp, ROHC_PROFILE_UDP);
	rohc_activate_profile(this->comp, ROHC_PROFILE_IP);
	rohc_activate_profile(this->comp, ROHC_PROFILE_UDPLITE);
	rohc_activate_profile(this->comp, ROHC_PROFILE_RTP);

	// create the ROHC decompressor and associate it with the
	// compressor to enable feedback
	this->decomp = rohc_alloc_decompressor(comp);
	if(this->decomp == NULL)
	{
		UTI_ERROR("%s cannot create ROHC decompressor\n", FUNCNAME);
		goto free_comp;
	}

	return;

free_comp:
	rohc_free_compressor(this->comp);
error:
	this->comp = NULL;
	this->decomp = NULL;
	;
}

RohcCtx::~RohcCtx()
{
	// free ROHC compressor/decompressor if created
	if(this->comp != NULL)
		rohc_free_compressor(this->comp);
	if(this->decomp != NULL)
		rohc_free_decompressor(this->decomp);
}

NetBurst *RohcCtx::encapsulate(NetPacket *packet,
                               int &context_id,
                               long &time)
{
	const char *FUNCNAME = "[RohcCtx::encapsulate]";
	NetBurst *rohc_packets;
	RohcPacket *rohc_packet;
	static unsigned char rohc_data[MAX_ROHC_SIZE];
	int rohc_len;

	context_id = 0;
	time = 0;

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s packet is not valid, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	// packet must be IPv4 or IPv6
	if(packet->type() != NET_PROTO_IPV4 &&
	   packet->type() != NET_PROTO_IPV6)
	{
		UTI_ERROR("%s packet is neither IPv4 nor IPv6, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	UTI_DEBUG("%s encapsulate a %d-byte packet of type 0x%04x\n",
	          FUNCNAME, packet->totalLength(), packet->type());

	// the ROHC compressor must be ready
	if(this->comp == NULL)
	{
		UTI_ERROR("%s ROHC compressor not ready, "
		          "drop packet\n", FUNCNAME);
		goto drop;
	}

	// compress the IP packet thanks to the ROHC library
	rohc_len = rohc_compress(this->comp,
	                         (unsigned char *) packet->data().c_str(),
	                         packet->totalLength(),
	                         rohc_data, MAX_ROHC_SIZE);
	if(rohc_len <= 0)
	{
		UTI_ERROR("%s ROHC compression failed, "
		          "drop packet\n", FUNCNAME);
		goto drop;
	}

	// create a ROHC packet from data computed by the ROHC library
	rohc_packet =  new RohcPacket(rohc_data, rohc_len);
	if(rohc_packet == NULL)
	{
		UTI_ERROR("%s cannot create ROHC packet, "
		          "drop the network packet\n", FUNCNAME);
		goto drop;
	}

	// create an empty burst of ROHC packets
	rohc_packets = new NetBurst();
	if(rohc_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst "
		          "of ROHC packets\n", FUNCNAME);
		goto clean;
	}

	// copy some parameters
	rohc_packet->setMacId(packet->macId());
	rohc_packet->setTalId(packet->talId());
	rohc_packet->setQos(packet->qos());

	// add ROHC packet to burst
	rohc_packets->push_back(rohc_packet);

	UTI_DEBUG("%s %d-byte %s packet/frame => %d-byte ROHC packet\n",
	          FUNCNAME, packet->totalLength(), packet->name().c_str(),
	          rohc_packet->totalLength());

	return rohc_packets;

clean:
	delete rohc_packet;
drop:
	return NULL;
}

NetBurst *RohcCtx::desencapsulate(NetPacket *packet)
{
	const char *FUNCNAME = "[RohcCtx::desencapsulate]";
	NetPacket *net_packet;
	NetBurst *net_packets;
	RohcPacket *rohc_packet;
	static unsigned char ip_data[MAX_ROHC_SIZE];
	int ip_len;

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s encapsulation packet is not valid, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	// packet must be a ROHC packet
	if(packet->type() != NET_PROTO_ROHC)
	{
		UTI_ERROR("%s encapsulation packet is not a ROHC packet "
		          "(type = 0x%04x), drop the packet\n",
		          FUNCNAME, packet->type());
		goto drop;
	}

	rohc_packet = (RohcPacket *) packet;

	// the ROHC decompressor must be ready
	if(this->decomp == NULL)
	{
		UTI_ERROR("%s ROHC decompressor not ready, "
		          "drop packet\n", FUNCNAME);
		goto drop;
	}

	// decompress the IP packet thanks to the ROHC library
	ip_len = rohc_decompress(this->decomp,
	                         (unsigned char *) packet->data().c_str(),
	                         packet->totalLength(),
	                         ip_data, MAX_ROHC_SIZE);
	if(ip_len <= 0)
	{
		UTI_ERROR("%s ROHC decompression failed, "
		          "drop packet\n", FUNCNAME);
		goto drop;
	}

	// create network packet according to type
	switch(IpPacket::version(ip_data, ip_len))
	{
		case 4:
			net_packet = new Ipv4Packet(ip_data, ip_len);
			break;
		case 6:
			net_packet = new Ipv6Packet(ip_data, ip_len);
			break;
		default:
			UTI_ERROR("%s unknown IP version\n", FUNCNAME);
			goto drop;
	}
	if(net_packet == NULL)
	{
		UTI_ERROR("%s cannot create IP packet, "
		          "drop the ROHC packet\n", FUNCNAME);
		goto drop;
	}

	// copy some parameters
	net_packet->setMacId(packet->macId());
	net_packet->setTalId(packet->talId());
	net_packet->setQos(packet->qos());

	// create an empty burst of network packets
	net_packets = new NetBurst();
	if(net_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of network packets\n",
		          FUNCNAME);
		goto clean;
	}

	// add network packet to burst
	net_packets->push_back(net_packet);

	UTI_DEBUG("%s %d-byte ROHC packet => %d-byte %s packet/frame\n",
	          FUNCNAME, rohc_packet->totalLength(),
	          net_packet->totalLength(), net_packet->name().c_str());

	return net_packets;

clean:
	delete net_packet;
drop:
	return NULL;
}

std::string RohcCtx::type()
{
	return std::string("ROHC");
}

NetBurst *RohcCtx::flush(int context_id)
{
	// nothing to do for ROHC
	UTI_DEBUG("[RohcCtx::flush] do nothing\n");
	return NULL;
}

NetBurst * RohcCtx::flushAll()
{
	// nothing to do for ROHC
	UTI_DEBUG("[RohcCtx::flushAll] do nothing\n");
	return NULL;
}
