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
 * @file UleCtx.cpp
 * @brief ULE encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "UleCtx.h"
#include "RohcPacket.h"

#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"


UleCtx::UleCtx(): EncapCtx(), mandatory_exts(), optional_exts(), build_exts()
{
	this->build_exts.clear();
}

UleCtx::~UleCtx()
{
	std::map < uint8_t, UleExt * >::iterator it;

	// delete all the mandatory extension handlers
	for(it = this->mandatory_exts.begin(); it != this->mandatory_exts.end(); it++)
		delete (*it).second;
	this->mandatory_exts.clear();

	// delete all the optional extension handlers
	for(it = this->optional_exts.begin(); it != this->optional_exts.end(); it++)
		delete (*it).second;
	this->optional_exts.clear();
}

bool UleCtx::addExt(UleExt *ext, bool activated)
{
	const char *FUNCNAME = "[UleCtx::addExt]";
	std::map < uint8_t, UleExt * > *exts;
	std::map < uint8_t, UleExt * >::iterator it;
	std::pair < std::map < uint8_t, UleExt * >::iterator, bool > infos;

	// check extension validity
	if(ext == NULL)
	{
		UTI_ERROR("%s invalid extension\n", FUNCNAME);
		goto bad;
	}

	// find the good extension list
	if(ext->isMandatory())
		exts = &(this->mandatory_exts);
	else
		exts = &(this->optional_exts);

	// check if an extension is already registered with the same magic number
	it = exts->find(ext->type());
	if(it != exts->end())
	{
		UTI_ERROR("%s %s extension 0x%02x already registered\n", FUNCNAME,
		          ext->isMandatory() ? "mandatory" : "optional", ext->type());
		goto bad;
	}

	// add extension to the list of mandatory extensions
	infos = exts->insert(std::make_pair(ext->type(), ext));
	if(!infos.second)
	{
		UTI_ERROR("%s cannot add %s extension 0x%02x\n", FUNCNAME,
		          ext->isMandatory() ? "mandatory" : "optional", ext->type());
		goto bad;
	}

	// add the extension to the build list if activated
	if(activated)
		this->build_exts.push_back(ext);

	return true;

bad:
	return false;
}

/*
 * +-+-------------------------------------------------------+--------+
 * |D| Length | Type | Dest Address* |           PDU         | CRC-32 |
 * +-+-------------------------------------------------------+--------+
 *
 * Destination Address field present if D = 1
 */
NetBurst * UleCtx::encapsulate(NetPacket *packet,
                               int &context_id,
                               long &time)
{
	const char *FUNCNAME = "[UleCtx::encapsulate]";
	NetBurst *ule_packets;
	UlePacket *ule_packet;
	uint16_t ptype;
	Data payload;
	std::list<UleExt *>::iterator it;

	context_id = 0;
	time = 0;

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s packet is not valid, drop the packet\n", FUNCNAME);
		goto drop;
	}

	UTI_DEBUG("%s encapsulate a %d-byte packet of type 0x%04x\n", FUNCNAME,
	          packet->totalLength(), packet->type());

	// add ULE extension headers if asked
	ptype = packet->type();
	payload = packet->data();
	for(it = this->build_exts.begin(); it != this->build_exts.end(); it++)
	{
		switch((*it)->build(ptype, payload))
		{
			case ULE_EXT_OK:
				UTI_DEBUG("%s %s ULE extension 0x%02x successfully built\n",
				          FUNCNAME, ((*it)->isMandatory() ? "mandatory" :
				          "optional"), (*it)->type());
				break;

			case ULE_EXT_DISCARD:
			case ULE_EXT_ERROR:
				UTI_ERROR("%s %s ULE extension 0x%02x build failed\n", FUNCNAME,
				          ((*it)->isMandatory() ? "mandatory" : "optional"),
				          (*it)->type());
				goto drop;
		}

		ptype = (*it)->payloadType();
		payload = (*it)->payload();

		UTI_DEBUG("%s next header: size = %u, type = 0x%04x\n", FUNCNAME,
		          payload.length(), ptype);
	}

	// create ULE packet with network packet (and extension headers) as payload
	// (type taken from network packet or extension header, no destination
	// address field)
	ule_packet =  new UlePacket(ptype, NULL, payload);
	if(ule_packet == NULL)
	{
		UTI_ERROR("%s cannot create ULE packet, drop the network packet\n",
		          FUNCNAME);
		goto drop;
	}

	// create an empty burst of ULE packets
	ule_packets = new NetBurst();
	if(ule_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of ULE packets\n",
		          FUNCNAME);
		goto clean;
	}

	// copy some parameters
	ule_packet->setMacId(packet->macId());
	ule_packet->setTalId(packet->talId());
	UTI_DEBUG_L3("%s talId of Ule packet: %ld \n", FUNCNAME, ule_packet->talId());
	ule_packet->setQos(packet->qos());

	// add ULE packet to burst
	ule_packets->push_back(ule_packet);

	UTI_DEBUG("%s %d-byte %s packet/frame => %d-byte ULE packet\n", FUNCNAME,
	          packet->totalLength(), packet->name().c_str(), ule_packet->totalLength());

	return ule_packets;

clean:
	delete ule_packet;
drop:
	return NULL;
}

/*
 * +-+-------------------------------------------------------+--------+
 * |D| Length | Type | Dest Address* |           PDU         | CRC-32 |
 * +-+-------------------------------------------------------+--------+
 *
 * Destination Address field present if D = 1
 */
NetBurst * UleCtx::desencapsulate(NetPacket *packet)
{
	const char *FUNCNAME = "[UleCtx::desencapsulate]";
	NetPacket *net_packet;
	NetBurst *net_packets;
	UlePacket *ule_packet;
	uint16_t ptype;
	Data payload;

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s encapsulation packet is not valid, drop the packet\n",
		          FUNCNAME);
		goto drop;
	}

	// packet must be an ULE packet
	if(packet->type() != NET_PROTO_ULE)
	{
		UTI_ERROR("%s encapsulation packet is not an ULE packet, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	// cast from a generic packet to an ULE packet
	ule_packet = dynamic_cast<UlePacket *>(packet);
	if(ule_packet == NULL)
	{
		UTI_ERROR("%s bad cast from NetPacket to UlePacket!\n", FUNCNAME);
		goto drop;
	}

	// decode ULE extension if present
	ptype = ule_packet->payloadType();
	payload = ule_packet->payload();

	while(ptype < 1536)
	{
		// one or more extensions are present
		uint8_t hlen;
		uint8_t htype;
		std::map < uint8_t, UleExt * > *exts;
		std::map < uint8_t, UleExt * >::iterator it;
		UleExt *ext;

		hlen = (ptype >> 8) & 0x07;
		htype =  ptype & 0xff;

		// mandatory or optional extension header ?
		if(hlen == 0)
			exts = &(this->mandatory_exts);
		if(hlen >= 6)
		{
			UTI_ERROR("%s bad length (0x%x) for ULE extension, drop packet\n",
			          FUNCNAME, hlen);
			goto drop;
		}
		else
			exts = &(this->optional_exts);

		// find extension handler
		it = exts->find(htype);
		if(it == exts->end())
		{
			UTI_ERROR("%s %s ULE extension 0x%02x is not supported, drop packet\n",
			          FUNCNAME, (hlen == 0 ? "mandatory" : "optional"), htype);
			goto drop;
		}
		ext = (*it).second;

		// decode the extension
		switch(ext->decode(hlen, payload))
		{
			case ULE_EXT_OK:
				UTI_DEBUG("%s %s ULE extension 0x%02x successfully decoded\n",
				          FUNCNAME, (hlen == 0 ? "mandatory" : "optional"), htype);
				break;

			case ULE_EXT_DISCARD:
				UTI_DEBUG("%s %s ULE extension 0x%02x successfully decoded, but "
				          "ULE packet must be discarded\n", FUNCNAME, (hlen == 0 ?
				          "mandatory" : "optional"), htype);
				goto discard;

			case ULE_EXT_ERROR:
				UTI_ERROR("%s analysis of %s ULE extension 0x%02x failed, drop "
				          "packet\n", FUNCNAME, (hlen == 0 ? "mandatory" :
				          "optional"), htype);
				goto drop;
		}

		// get the new payload and the new payload type
		payload = ext->payload();
		ptype = ext->payloadType();

		UTI_DEBUG("%s next header: size = %u, type = 0x%04x\n", FUNCNAME,
		          payload.length(), ptype);
	}

	if(this->type().find("ULE/ROHC") != std::string::npos)
	{
		net_packet = new RohcPacket(payload);
		UTI_DEBUG("%s ULE payload is ROHC packet\n", FUNCNAME);
	}
	else
	{
		switch(ptype)
		{
			case NET_PROTO_IPV4:
					net_packet = new Ipv4Packet(payload);
				break;
			case NET_PROTO_IPV6:
				net_packet = new Ipv6Packet(payload);
				break;
			default:
				UTI_ERROR("%s ULE payload type is not supported (0x%04x)\n",
				          FUNCNAME, ptype);
				goto drop;
		}
	}
	if(net_packet == NULL)
	{
		UTI_ERROR("%s cannot create network packet, drop the ULE packet\n",
		          FUNCNAME);
		goto drop;
	}

	// copy some parameters
	net_packet->setQos(ule_packet->qos());
	net_packet->setMacId(ule_packet->macId());
	net_packet->setTalId(ule_packet->talId());

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

	UTI_DEBUG("%s %d-byte ULE packet => %d-byte %s packet/frame\n", FUNCNAME,
	          ule_packet->totalLength(), net_packet->totalLength(), net_packet->name().c_str());

	return net_packets;

discard:
	// ULE packet discarded, so return an empty burst
   net_packets = new NetBurst();
   if(net_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of network packets\n",
		          FUNCNAME);
		goto drop;
	}
	UTI_DEBUG("%s %d-byte ULE packet => discarded\n", FUNCNAME,
	          ule_packet->totalLength());
	return net_packets;

clean:
	delete net_packet;
drop:
	return NULL;
}

std::string UleCtx::type()
{
	return std::string("ULE");
}

NetBurst * UleCtx::flush(int context_id)
{
	// nothing to do for ULE
	UTI_DEBUG("[UleCtx::flush] do nothing\n");
	return NULL;
}

NetBurst * UleCtx::flushAll()
{
	// nothing to do for ULE
	UTI_DEBUG("[UleCtx::flushAll] do nothing\n");
	return NULL;
}
