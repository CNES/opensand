/**
 * @file GseRohcCtx.cpp
 * @brief GSE encapsulation / deencapsulation context
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#include "GseRohcCtx.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"
#include <vector>
#include <map>
#include <assert.h>

GseRohcCtx::GseRohcCtx(int qos_nbr,
                       unsigned int packing_threshold,
                       unsigned int packet_length):
  RohcCtx(), GseCtx(qos_nbr, packing_threshold, packet_length)
{
}

GseRohcCtx::~GseRohcCtx()
{
}

NetBurst *GseRohcCtx::encapsulate(NetPacket *packet,
                                  int &context_id,
                                  long &time)
{
	const char *FUNCNAME = "[GseRohcCtx::encapsulate]";
	NetBurst *rohc_packets;
	NetBurst *gse_packets;

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s packet is not valid, drop the packet\n", FUNCNAME);
		goto drop;
	}

	// ROHC encapsulation
	rohc_packets = this->RohcCtx::encapsulate(packet, context_id, time);
	if(rohc_packets == NULL)
	{
		UTI_ERROR("%s ROHC encapsulation failed, drop packet\n", FUNCNAME);
		goto drop;
	}

	// ROHC encapsulation should not produce more than one ROHC packet
	if(rohc_packets->length() != 1)
	{
		UTI_ERROR("%s ROHC encapsulation produced too few or too many "
		          "ROHC packets (%d), drop packet\n", FUNCNAME,
		          rohc_packets->length());
		goto clean_rohc;
	}

	// GSE encapsulation
	gse_packets = this->GseCtx::encapsulate(rohc_packets->front(),
	                                        context_id, time);
	if(gse_packets == NULL)
	{
		UTI_ERROR("%s GSE encapsulation failed, drop packet\n", FUNCNAME);
		goto clean_rohc;
	}

	// clean temporary burst of ROHC packets
	delete rohc_packets;

	UTI_DEBUG("%s GSE/ROHC encapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 %s packet/frame => %d GSE/ROHC frames\n", FUNCNAME,
	          packet->name().c_str(), gse_packets->length());

	return gse_packets;

clean_rohc:
	delete rohc_packets;
drop:
	return NULL;

}

NetBurst *GseRohcCtx::desencapsulate(NetPacket *packet)
{
	const char *FUNCNAME = "[AtmAal5RohcCtx::desencapsulate]";
	NetBurst *rohc_packets;
	NetBurst::iterator it;
	NetBurst *all_net_packets;
	NetBurst *net_packets;

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s encapsulation packet is not valid, drop the packet\n",
		          FUNCNAME);
		goto drop;
	}

	// packet must be a GSE packet
	if(packet->type() != NET_PROTO_GSE)
	{
		UTI_ERROR("%s encapsulation packet is not an GSE packet, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	// GSE desencapsulation
	rohc_packets = this->GseCtx::desencapsulate(packet);
	if(rohc_packets == NULL)
	{
		UTI_ERROR("%s GSE desencapsulation failed, drop packet\n", FUNCNAME);
		goto drop;
	}

	// create an empty burst of network packets
	all_net_packets = new NetBurst();
	if(all_net_packets == NULL)
	{
		UTI_ERROR("%s cannot create a burst of packets, drop packet\n", FUNCNAME);
		goto clean_rohc;
	}

	// ROHC desencapsulation
	for(it = rohc_packets->begin(); it != rohc_packets->end(); it++)
	{
		net_packets = this->RohcCtx::desencapsulate(*it);
		if(net_packets == NULL)
		{
			UTI_ERROR("%s ROHC desencapsulation failed, drop packet\n", FUNCNAME);
			continue;
		}

		// ROHC desencapsulation should not produce more than one network packet
		if(net_packets->length() != 1)
		{
			UTI_ERROR("%s ROHC desencapsulation produced too many network packets "
			          "(%d), drop packet\n", FUNCNAME, net_packets->length());
			delete net_packets;
			continue;
		}

		// add network packet to the final burst
		all_net_packets->push_back(net_packets->front());

		// delete the temporary burst of network packets
		net_packets->clear();
		delete net_packets;
	}

	// clean temporary burst of ROHC packets
	delete rohc_packets;

	UTI_DEBUG("%s GSE/ROHC desencapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 GSE packet => %d %s packets/frames\n", FUNCNAME,
	          all_net_packets->length(), all_net_packets->name().c_str());

	return all_net_packets;

clean_rohc:
	delete rohc_packets;
drop:
	return NULL;
}

std::string GseRohcCtx::type()
{
	return std::string("GSE/ROHC");
}

NetBurst *GseRohcCtx::flush(int context_id)
{
	const char *FUNCNAME = "[GseRohcCtx::flush]";
	NetBurst *gse_packets;

	// flush corresponding GSE context
	gse_packets = this->GseCtx::flush(context_id);
	if(gse_packets == NULL)
	{
		UTI_ERROR("%s flushing GSE context %d failed\n", FUNCNAME, context_id);
		goto error;
	}

	UTI_DEBUG("%s %d GSE frames flushed from context %d\n", FUNCNAME,
	          gse_packets->length(), context_id);

	return gse_packets;

error:
	return NULL;

}

NetBurst *GseRohcCtx::flushAll()
{
	const char *FUNCNAME = "[GseRohcCtx::flushAll]";
	NetBurst *gse_packets;

	// flush all GSE contexts
	gse_packets = this->GseCtx::flushAll();
	if(gse_packets == NULL)
	{
		UTI_ERROR("%s flushing all GSE contexts failed\n", FUNCNAME);
		goto error;
	}

	UTI_DEBUG("%s %d GSE frames flushed from GSE contexts\n", FUNCNAME,
	          gse_packets->length());

	return gse_packets;

error:
	return NULL;
}

