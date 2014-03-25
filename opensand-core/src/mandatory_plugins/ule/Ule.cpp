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
 * @file Ule.cpp
 * @brief ULE encapsulation plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#include "Ule.h"

#include <opensand_output/Output.h>

#include <vector>
#include <map>

#include "UlePacket.h"

Ule::Ule():
	EncapPlugin(NET_PROTO_ULE)
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


Ule::Context::Context(EncapPlugin &plugin):
	EncapPlugin::EncapContext(plugin),
	mandatory_exts(), optional_exts(), build_exts()
{
}

void Ule::Context::init()
{
	EncapPlugin::EncapContext::init();
	this->build_exts.clear();
	// TODO extension table in configuration
	// TODO boolean for crc in configuration
#if 0
	bool enable_ext = false;
	if(enable_ext)
	{
		UleExt *ext;

		// create Test SNDU ULE extension
		ext = new UleExtTest();
		if(ext == NULL)
		{
			LOG(this->log, LEVEL_ERROR,
			    "failed to create Test SNDU ULE extension\n");
		}
		else
		{
			// add Test SNDU ULE extension but do not enable it
			if(!this->addExt(ext, false))
			{
				LOG(this->log, LEVEL_ERROR,
				    "failed to add Test SNDU ULE extension\n");
				delete ext;
			}
		}
		// create Security ULE extension
		ext = new UleExtSecurity();
		if(ext == NULL)
		{
			LOG(this->log, LEVEL_ERROR,
			    "failed to create Padding ULE extension\n");
		}
		else
		{
			// add Security ULE extension and enable it
			if(!this->addExt(ext, true))
			{
				LOG(this->log, LEVEL_ERROR,
				    "failed to add Padding ULE extension\n");
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
	NetBurst *ule_packets = NULL;
	NetBurst::iterator packet;

	// create an empty burst of ULE packets
	ule_packets = new NetBurst();
	if(ule_packets == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of ULE packets\n");
		delete burst;
		return NULL;
	}

	for(packet = burst->begin(); packet != burst->end(); packet++)
	{
		if(!this->encapUle(*packet, ule_packets))
		{
			LOG(this->log, LEVEL_ERROR,
			    "ULE encapsulation failed, drop packet\n");
			continue;
		}
	}

	// delete the burst and all packets in it
	delete burst;
	return ule_packets;
}


NetBurst *Ule::Context::deencapsulate(NetBurst *burst)
{
	NetBurst *net_packets;

	NetBurst::iterator packet;

	// create an empty burst of network packets
	net_packets = new NetBurst();
	if(net_packets == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of network "
		    "packets\n");
		delete burst;
		return false;
	}

	for(packet = burst->begin(); packet != burst->end(); packet++)
	{
		// packet must be valid
		if(*packet == NULL)
		{
			LOG(this->log, LEVEL_ERROR,
			    "encapsulation packet is not valid, drop "
			    "the packet\n");
			continue;
		}

		// packet must be an ULE packet
		if((*packet)->getType() != this->getEtherType())
		{
			LOG(this->log, LEVEL_ERROR,
			    "encapsulation packet is not an ULE packet "
			    "(type = 0x%04x), drop the packet\n",
			    (*packet)->getType());
			continue;
		}

		// No filtering in ULE, since it is done in the lower encap scheme

		if(!this->deencapUle(*packet, net_packets))
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot create a burst of packets, drop "
			    "packet\n");
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
	UlePacket *ule_packet;
	uint16_t ptype;
	Data payload;
	std::list<UleExt *>::iterator it;
	// keep the destination spot
	uint16_t dest_spot = packet->getDstSpot();

	LOG(this->log, LEVEL_INFO,
	    "encapsulate a %zu-byte packet of type 0x%04x\n",
	    packet->getTotalLength(), packet->getType());

	// add ULE extension headers if asked
	ptype = packet->getType();
	payload = packet->getData();
	for(it = this->build_exts.begin(); it != this->build_exts.end(); it++)
	{
		switch((*it)->build(ptype, payload))
		{
			case ULE_EXT_OK:
				LOG(this->log, LEVEL_INFO,
				    "%s ULE extension 0x%02x successfully "
				    "built\n",
				    ((*it)->isMandatory() ? "mandatory" :
				    "optional"), (*it)->type());
				break;

			case ULE_EXT_DISCARD:
			case ULE_EXT_ERROR:
				LOG(this->log, LEVEL_ERROR,
				    "%s ULE extension 0x%02x build failed\n",
				    ((*it)->isMandatory() ? "mandatory" : "optional"),
				    (*it)->type());
				goto drop;
		}

		ptype = (*it)->payloadType();
		payload = (*it)->payload();

		LOG(this->log, LEVEL_INFO,
		    "next header: size = %zu, type = 0x%04x\n",
		    payload.length(), ptype);
	}

	// create ULE packet with network packet (and extension headers) as payload
	// (type taken from network packet or extension header, no destination
	// address field)
	ule_packet =  new UlePacket(ptype, NULL, payload, this->enable_crc);
	if(ule_packet == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot create ULE packet, drop the network "
		    "packet\n");
		goto drop;
	}
	ule_packet->setSrcTalId(packet->getSrcTalId());
	ule_packet->setDstTalId(packet->getDstTalId());
	ule_packet->setQos(packet->getQos());

	// set the destination spot ID
	ule_packet->setDstSpot(dest_spot);
	// add ULE packet to burst
	ule_packets->add(ule_packet);

	LOG(this->log, LEVEL_INFO,
	    "%zu-byte %s packet/frame => %zu-byte ULE packet\n",
	    packet->getTotalLength(), packet->getName().c_str(),
	    ule_packet->getTotalLength());

	return true;

drop:
	return false;
}

bool Ule::Context::deencapUle(NetPacket *packet, NetBurst *net_packets)
{
	NetPacket *net_packet=NULL;
	UlePacket *ule_packet=NULL;
	uint16_t ptype;
	Data payload;
	// keep the destination spot
	uint16_t dest_spot = packet->getDstSpot();

	// packet must be an ULE packet
	if(packet->getType() != NET_PROTO_ULE)
	{
		LOG(this->log, LEVEL_ERROR,
		    "encapsulation packet is not an ULE packet, "
		    "drop the packet\n");
		goto error;
	}

	// cast from a generic packet to an ULE packet
	ule_packet = new UlePacket(packet->getData());
	if(ule_packet == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot create UlePacket from NetPacket\n");
		goto error;
	}
	if(!ule_packet->isValid(this->enable_crc))
	{
		LOG(this->log, LEVEL_ERROR,
		    "ULE packet is invalid\n");
		goto discard;
	}

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
			LOG(this->log, LEVEL_ERROR,
			    "bad length (0x%x) for ULE extension, drop "
			    "packet\n", hlen);
			goto discard;
		}
		else
			exts = &(this->optional_exts);

		// find extension handler
		it = exts->find(htype);
		if(it == exts->end())
		{
			LOG(this->log, LEVEL_ERROR,
			    "%s ULE extension 0x%02x is not supported, "
			    "drop packet\n",
			    (hlen == 0 ? "mandatory" : "optional"), htype);
			goto discard;
		}
		ext = (*it).second;

		// decode the extension
		switch(ext->decode(hlen, payload))
		{
			case ULE_EXT_OK:
				LOG(this->log, LEVEL_INFO,
				    "%s ULE extension 0x%02x successfully "
				    "decoded\n", 
				    (hlen == 0 ? "mandatory" : "optional"), htype);
				break;

			case ULE_EXT_DISCARD:
				LOG(this->log, LEVEL_INFO,
				    "%s ULE extension 0x%02x successfully "
				    "decoded, but ULE packet must be discarded\n",
				    (hlen == 0 ? "mandatory" : "optional"), htype);
				goto discard;

			case ULE_EXT_ERROR:
				LOG(this->log, LEVEL_ERROR,
				    "analysis of %s ULE extension 0x%02x "
				    "failed, drop packet\n",
				    (hlen == 0 ? "mandatory" : "optional"), htype);
				goto discard;
		}

		// get the new payload and the new payload type
		payload = ext->payload();
		ptype = ext->payloadType();

		LOG(this->log, LEVEL_INFO,
		    "next header: size = %zu, type = 0x%04x\n",
		    payload.length(), ptype);
	}

	LOG(this->log, LEVEL_INFO,
	    "received a packet with type 0x%.4x\n", ptype);

	net_packet = this->current_upper->build(payload,
	                                        payload.length(),
	                                        packet->getQos(),
	                                        packet->getSrcTalId(), packet->getDstTalId());
	if(net_packet == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot create a %s packet, drop the ULE packet\n",
		    this->current_upper->getName().c_str());
		goto discard;
	}

	// set the destination spot ID
	net_packet->setDstSpot(dest_spot);
	// add network packet to burst
	net_packets->add(net_packet);

	LOG(this->log, LEVEL_INFO,
	    "%zu-byte ULE packet => %zu-byte %s packet/frame\n",
	    ule_packet->getTotalLength(),
	    net_packet->getTotalLength(),
	    net_packet->getName().c_str());

	delete ule_packet;
	return true;

discard:
	delete ule_packet;
error:
	return false;
}


bool Ule::Context::addExt(UleExt *ext, bool activated)
{
	std::map < uint8_t, UleExt * > *exts;
	std::map < uint8_t, UleExt * >::iterator it;
	std::pair < std::map < uint8_t, UleExt * >::iterator, bool > infos;

	// check extension validity
	if(ext == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "invalid extension\n");
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
		LOG(this->log, LEVEL_ERROR,
		    "%s extension 0x%02x already registered\n",
		    ext->isMandatory() ? "mandatory" : "optional",
		    ext->type());
		goto bad;
	}

	// add extension to the list of mandatory extensions
	infos = exts->insert(std::make_pair(ext->type(), ext));
	if(!infos.second)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot add %s extension 0x%02x\n",
		    ext->isMandatory() ? "mandatory" : "optional",
		    ext->type());
		goto bad;
	}

	// add the extension to the build list if activated
	if(activated)
		this->build_exts.push_back(ext);

	return true;

bad:
	return false;
}


NetPacket *Ule::PacketHandler::build(const Data &data, size_t data_length,
                                     uint8_t qos,
                                     uint8_t src_tal_id, uint8_t dst_tal_id) const
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


