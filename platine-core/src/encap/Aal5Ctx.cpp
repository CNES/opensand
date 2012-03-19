/**
 * @file Aal5Ctx.cpp
 * @brief AAL5 encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "Aal5Ctx.h"

#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"


Aal5Ctx::Aal5Ctx(): EncapCtx()
{
}

Aal5Ctx::~Aal5Ctx()
{
}

NetBurst *Aal5Ctx::encapsulate(NetPacket *packet,
                                int &context_id,
                                long &time)
{
	const char *FUNCNAME = "[Aal5Ctx::encapsulate]";
	Aal5Packet *aal5_packet;
	NetBurst *aal5_packets;

	time = 0; // no need for an encapsulation timer

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s packet is not valid, drop the packet\n",
		          FUNCNAME);
		goto drop;
	}

	UTI_DEBUG("%s packet is valid, build an AAL5 packet\n",
	          FUNCNAME);

	// build an AAL5 packet with a network packet as payload
	aal5_packet = Aal5Packet::createFromPayload(packet->data());
	if(aal5_packet == NULL)
	{
		UTI_ERROR("%s cannot create an AAL5 packet, "
		          "drop the network packet\n", FUNCNAME);
		goto drop;
	}

	// check AAL5 packet validity
	if(!aal5_packet->isValid())
	{
		UTI_ERROR("%s AAL5 packet is not valid, "
		          "drop the network packet\n", FUNCNAME);
		goto clean;
	}

	// copy some parameters
	aal5_packet->setMacId(packet->macId());
	aal5_packet->setTalId(packet->talId());
	aal5_packet->setQos(packet->qos());

	UTI_DEBUG("%s AAL5 packet is valid (QoS %d)\n",
	          FUNCNAME, aal5_packet->qos());

	// create an empty burst of AAL5 packets
	aal5_packets = new NetBurst();
	if(aal5_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst "
		          "of AAL5 packets\n", FUNCNAME);
		goto clean;
	}

	// add the AAL5 packet to the list
	aal5_packets->push_back(aal5_packet);

	return aal5_packets;

clean:
	delete aal5_packet;
drop:
	return NULL;
}

NetBurst *Aal5Ctx::desencapsulate(NetPacket *packet)
{
	const char *FUNCNAME = "[Aal5Ctx::desencapsulate]";
	NetPacket *ip_packet;
	NetBurst *ip_packets = NULL;
	Aal5Packet *aal5_packet;
	Data aal5_payload;

	// packet must be a valid encapsulation packet
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s encapsulation packet is not valid, "
		          "drop packet\n", FUNCNAME);
		goto drop;
	}

	// packet must be an AAL5 packet
	if(packet->type() != NET_PROTO_AAL5)
	{
		UTI_ERROR("%s encapsulation packet is not an AAL5 packet, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	// cast from a generic packet to an AAL5 packet
	aal5_packet = dynamic_cast<Aal5Packet *>(packet);
	if(aal5_packet == NULL)
	{
		UTI_ERROR("%s bad cast from NetPacket to Aal5Packet!\n",
		          FUNCNAME);
		goto drop;
	}

	aal5_payload = aal5_packet->payload();

	if(this->type().find("AAL5/ROHC") != std::string::npos)
	{
		ip_packet = new RohcPacket(aal5_payload);
		UTI_DEBUG("%s AAL5 payload is ROHC packet\n", FUNCNAME);
	}
	else
	{
		switch(IpPacket::version(aal5_payload))
		{
			case 4:
				ip_packet = new Ipv4Packet(aal5_payload);
				break;
			case 6:
				ip_packet = new Ipv6Packet(aal5_payload);
				break;
			default:
				UTI_ERROR("%s AAL5 payload is neither IPv4 nor IPv6, "
				          "drop packet\n", FUNCNAME);
				goto drop;
		}
	}

	// check IPv[46] packet validity
	if(ip_packet == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for IP packet\n",
		          FUNCNAME);
		goto drop;
	}

	// copy some parameters
	ip_packet->setMacId(aal5_packet->macId());
	ip_packet->setTalId(aal5_packet->talId());
	ip_packet->setQos(aal5_packet->qos());

	// create an empty burst of IP packets
	ip_packets = new NetBurst();
	if(ip_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst "
		          "of IP packets\n", FUNCNAME);
		goto clean;
	}

	// add the IP packet to the list
	ip_packets->push_back(ip_packet);

	UTI_DEBUG("%s %s packet added to the burst\n", FUNCNAME,
	          ip_packet->name().c_str());

	return ip_packets;

clean:
	delete ip_packet;
drop:
	return NULL;
}

std::string Aal5Ctx::type()
{
	return std::string("AAL5");
}

NetBurst *Aal5Ctx::flush(int context_it)
{
	// nothing to do for AAL5
	UTI_DEBUG("[Aal5Ctx::flush] do nothing\n");
	return NULL;
}

NetBurst *Aal5Ctx::flushAll()
{
	// nothing to do for AAL5
	UTI_DEBUG("[Aal5Ctx::flushAll] do nothing\n");
	return NULL;
}

