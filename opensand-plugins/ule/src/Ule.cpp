/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 * @file Ule.cpp
 * @brief ULE encapsulation plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#include "Ule.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include <opensand_conf/uti_debug.h>
#include <vector>
#include <map>

#include "UlePacket.h"

Ule::Ule():
	EncapPlugin()
{
	this->ether_type = NET_PROTO_ULE;
	this->encap_name = "ULE";

	this->upper[TRANSPARENT].push_back("ROHC");
	this->upper[TRANSPARENT].push_back("IP");
	this->upper[REGENERATIVE].push_back("ROHC");
	this->upper[REGENERATIVE].push_back("IP");
}


Ule::Context::Context(EncapPlugin &plugin):
	EncapPlugin::EncapContext(plugin),
	mandatory_exts(), optional_exts(), build_exts()
{
	this->build_exts.clear();
	// TODO extension table in configuration
	// TODO boolean for crc in configuration
#if 0
	const char *FUNCNAME = "[Ule::Context::Context]";
	bool enable_ext = false;
	if(enable_ext)
	{
		UleExt *ext;

		// create Test SNDU ULE extension
		ext = new UleExtTest();
		if(ext == NULL)
		{   
			UTI_ERROR("%s failed to create Test SNDU ULE extension\n", FUNCNAME);
		}   
		else
		{
			// add Test SNDU ULE extension but do not enable it
			if(!this->addExt(ext, false))
			{   
				UTI_ERROR("%s failed to add Test SNDU ULE extension\n", FUNCNAME);
				delete ext;
			}   
		}
		// create Security ULE extension
		ext = new UleExtSecurity();
		if(ext == NULL)
		{   
			UTI_ERROR("%s failed to create Padding ULE extension\n", FUNCNAME);
		}   
		else
		{
			// add Security ULE extension and enable it
			if(!this->addExt(ext, true))
			{   
				UTI_ERROR("%s failed to add Padding ULE extension\n", FUNCNAME);
				delete ext;
			}   
		}
	}
#endif
	this->enable_crc = false;
}

Ule::Context::~Context()
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

NetBurst *Ule::Context::encapsulate(NetBurst *burst,
                                    std::map<long, int> &UNUSED(time_contexts))
{
	const char *FUNCNAME = "[Ule::Context::encapsulate]";
	NetBurst *ule_packets = NULL;
	NetBurst::iterator packet;

	// create an empty burst of ULE packets
	ule_packets = new NetBurst();
	if(ule_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of ULE packets\n",
		          FUNCNAME);
		delete burst;
		return NULL;
	}

	for(packet = burst->begin(); packet != burst->end(); packet++)
	{
		if(!this->encapUle(*packet, ule_packets))
		{
			UTI_ERROR("%s ULE encapsulation failed, drop packet\n", FUNCNAME);
			continue;
		}
	}

	// delete the burst and all packets in it
	delete burst;
	return ule_packets;
}


NetBurst *Ule::Context::deencapsulate(NetBurst *burst)
{
	const char *FUNCNAME = "[Ule::Context::deencapsulate]";
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
		// packet must be valid
		if(*packet == NULL)
		{
			UTI_ERROR("%s encapsulation packet is not valid, drop the packet\n",
			          FUNCNAME);
			continue;
		}

		// packet must be an ULE packet
		if((*packet)->getType() != this->getEtherType())
		{
			UTI_ERROR("%s encapsulation packet is not an ULE packet "
			          "(type = 0x%04x), drop the packet\n",
			          FUNCNAME, (*packet)->getType());
			continue;
		}

		// No filtering in ULE, since it is done in the lower encap scheme

		if(!this->deencapUle(*packet, net_packets))
		{
			UTI_ERROR("%s cannot create a burst of packets, drop packet\n", FUNCNAME);
			continue;
		}
	}

	// delete the burst and all packets in it
	delete burst;
	return net_packets;
}


/*
 * +-+-------------------------------------------------------+--------+
 * |D| Length | Type | Dest Address* |           PDU         | CRC-32 |
 * +-+-------------------------------------------------------+--------+
 *
 * Destination Address field present if D = 1
 */
bool Ule::Context::encapUle(NetPacket *packet,
                            NetBurst *ule_packets)
{
	const char *FUNCNAME = "Ule::Context::encapUle";
	UlePacket *ule_packet;
	uint16_t ptype;
	Data payload;
	std::list<UleExt *>::iterator it;
	// keep the destination spot
	uint16_t dest_spot = packet->getDstSpot();

	packet->addTrace(HERE());
	UTI_DEBUG("%s encapsulate a %d-byte packet of type 0x%04x\n", FUNCNAME,
	          packet->getTotalLength(), packet->getType());

	// add ULE extension headers if asked
	ptype = packet->getType();
	payload = packet->getData();
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
	ule_packet =  new UlePacket(ptype, NULL, payload, this->enable_crc);
	if(ule_packet == NULL)
	{
		UTI_ERROR("%s cannot create ULE packet, drop the network packet\n",
		          FUNCNAME);
		goto drop;
	}
	ule_packet->setSrcTalId(packet->getSrcTalId());
	ule_packet->setDstTalId(packet->getDstTalId());
	ule_packet->setQos(packet->getQos());

	// set the destination spot ID
	ule_packet->setDstSpot(dest_spot);
	// add ULE packet to burst
	ule_packets->add(ule_packet);

	UTI_DEBUG("%s %d-byte %s packet/frame => %d-byte ULE packet\n", FUNCNAME,
	          packet->getTotalLength(), packet->getName().c_str(),
	          ule_packet->getTotalLength());

	return true;

drop:
	return false;
}

bool Ule::Context::deencapUle(NetPacket *packet, NetBurst *net_packets)
{
	const char *FUNCNAME = "[Ule::Context::deencapUle]";
	NetPacket *net_packet;
	UlePacket *ule_packet;
	uint16_t ptype;
	Data payload;
	// keep the destination spot
	uint16_t dest_spot = packet->getDstSpot();

	packet->addTrace(HERE());

	// packet must be an ULE packet
	if(packet->getType() != NET_PROTO_ULE)
	{
		UTI_ERROR("%s encapsulation packet is not an ULE packet, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	// cast from a generic packet to an ULE packet
	ule_packet = new UlePacket(packet->getData());
	if(ule_packet == NULL)
	{
		UTI_ERROR("%s cannot create UlePacket from NetPacket\n", FUNCNAME);
		goto error;
	}
	ule_packet->addTrace(HERE());

	// decode ULE extension if present
	ptype = ule_packet->getPayloadType();
	payload = ule_packet->getPayload();

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

	if(ptype != this->current_upper->getEtherType())
	{
		// check if this is an IP packet (current_upper do not know the type)
		if((ptype == NET_PROTO_IPV4 ||
			ptype == NET_PROTO_IPV6) &&
		   this->current_upper->getName() != "IP")
		{
			UTI_ERROR("%s wrong packet type received (%u instead of %u)\n",
			          FUNCNAME, ptype, this->current_upper->getEtherType());
			goto drop;
		}
	}
	
	net_packet = this->current_upper->build((unsigned char *)(payload.c_str()),
	                                        payload.length(),
	                                        packet->getQos(),
	                                        packet->getSrcTalId(), packet->getDstTalId());
	if(net_packet == NULL)
	{
		UTI_ERROR("%s cannot create a %s packet, drop the ULE packet\n",
		          FUNCNAME, this->current_upper->getName().c_str());
		goto drop;
	}

	// set the destination spot ID
	net_packet->setDstSpot(dest_spot);
	// add network packet to burst
	net_packets->add(net_packet);

	UTI_DEBUG("%s %d-byte ULE packet => %d-byte %s packet/frame\n", FUNCNAME,
	          ule_packet->getTotalLength(), net_packet->getTotalLength(),
	          net_packet->getName().c_str());

	delete ule_packet;
	return true;

discard:
	delete ule_packet;
drop:
	delete net_packet;
error:
	return false;
}


bool Ule::Context::addExt(UleExt *ext, bool activated)
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


NetPacket *Ule::PacketHandler::build(unsigned char *data, size_t data_length,
                                     uint8_t qos,
                                     uint8_t src_tal_id, uint8_t dst_tal_id)
{
	return new NetPacket(data, data_length,
	                     this->getName(), this->getEtherType(),
	                     qos, src_tal_id, dst_tal_id, ULE_HEADER_LEN);
}


size_t Ule::PacketHandler::getLength(const unsigned char *data) const
{
	unsigned int len;

	// header fields
	len = ULE_HEADER_LEN;

	// destination address field ?
	if(((data[0] >> 7) & 0x01) == 0)
		len += ULE_ADDR_LEN;

	// payload + CRC
	len += ((data[0] & 0x7f) << 8) + (data[1] & 0xff);

	return len;
}

Ule::PacketHandler::PacketHandler(EncapPlugin &plugin):
	EncapPlugin::EncapPacketHandler(plugin)
{
}


