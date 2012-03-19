/**
 * @file GseAtmAal5RohcCtx.cpp
 * @brief GSE/ATM/AAL5/ROHC encapsulation / desencapsulation context
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#include "GseAtmAal5RohcCtx.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"


GseAtmAal5RohcCtx::GseAtmAal5RohcCtx(int qos_nbr, unsigned int packing_threshold):
  RohcCtx(),
  GseAtmAal5Ctx(qos_nbr, packing_threshold)
{
}

GseAtmAal5RohcCtx::~GseAtmAal5RohcCtx()
{
}

NetBurst *GseAtmAal5RohcCtx::encapsulate(NetPacket *packet,
                                         int &context_id,
                                         long &time)
{
	const char *FUNCNAME = "[GseAtmAal5RohcCtx::encapsulate]";
	NetBurst *rohc_packets;
	NetBurst::iterator it;
	NetBurst *all_gse_packets;
	NetBurst *gse_packets;

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s packet is not valid, drop the packet\n",
		          FUNCNAME);
		goto drop;
	}

	// ROHC encapsulation
	rohc_packets = RohcCtx::encapsulate(packet, context_id, time);
	if(rohc_packets == NULL)
	{
		UTI_ERROR("%s ROHC encapsulation failed, drop packet\n",
		          FUNCNAME);
		goto drop;
	}

	// create an empty burst of GSE packets
	all_gse_packets = new NetBurst();
	if(all_gse_packets == NULL)
	{
		UTI_ERROR("%s cannot create a burst of packets, drop packet\n",
		          FUNCNAME);
		goto clean_rohc;
	}

	// GSE/ATM/AAL5 encapsulation
	for(it = rohc_packets->begin(); it != rohc_packets->end(); it++)
	{
		// for GSE encapsulation we need to get the next protocol type
		(*it)->setType(NET_PROTO_ATM);

		gse_packets = GseAtmAal5Ctx::encapsulate(*it, context_id, time);
		if(gse_packets == NULL)
		{
			UTI_ERROR("%s GSE/ATM/AAL5 encapsulation failed, "
			          "drop packet\n", FUNCNAME);
			continue;
		}

		all_gse_packets->splice(all_gse_packets->end(), *gse_packets);

		// clean temporary burst of GSE packets
		gse_packets->clear();
		delete gse_packets;
	}

	// clean temporary burst of ATM cells
	delete rohc_packets;

	UTI_DEBUG("%s GSE/ATM/AAL5/ROHC encapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 %s packet/frame => %d GSE/ATM/AAL5/RCOH frames\n", FUNCNAME,
	          packet->name().c_str(), all_gse_packets->length());

	return all_gse_packets;

clean_rohc:
	delete rohc_packets;
drop:
	return NULL;
}

NetBurst *GseAtmAal5RohcCtx::desencapsulate(NetPacket *packet)
{
	const char *FUNCNAME = "[GseAtmAal5RohcCtx::desencapsulate]";
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
		UTI_ERROR("%s encapsulation packet is not a GSE packet, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	// GSE/ATM/AAL5 desencapsulation
	rohc_packets = GseAtmAal5Ctx::desencapsulate(packet);
	if(rohc_packets == NULL)
	{
		UTI_ERROR("%s GSE/ATM/AAL5 desencapsulation failed, "
		          "drop packet\n", FUNCNAME);
		goto drop;
	}

	// create an empty burst of network packets
	all_net_packets = new NetBurst();
	if(all_net_packets == NULL)
	{
		UTI_ERROR("%s cannot create a burst of packets, "
		          "drop packet\n", FUNCNAME);
		goto clean_rohc;
	}

	// ROHC desencapsulation
	for(it = rohc_packets->begin(); it != rohc_packets->end(); it++)
	{
		net_packets = RohcCtx::desencapsulate(*it);
		if(net_packets == NULL)
		{
			UTI_ERROR("%s ROHC desencapsulation failed, drop packet\n",
			          FUNCNAME);
			continue;
		}

		all_net_packets->splice(all_net_packets->end(), *net_packets);

		// clean temporary burst of network packets extracted from ATM cells
		net_packets->clear();
		delete net_packets;
	}

	// clean temporary burst of ATM cells
	delete rohc_packets;

	UTI_DEBUG("%s GSE/ATM/AAL5/ROHC desencapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 GSE frame => %d %s packets/frames\n", FUNCNAME,
	          all_net_packets->length(), all_net_packets->name().c_str());

	return all_net_packets;

clean_rohc:
	delete rohc_packets;
drop:
	return NULL;
}

std::string GseAtmAal5RohcCtx::type()
{
	return std::string("GSE/ATM/AAL5/ROHC");
}

NetBurst *GseAtmAal5RohcCtx::flush(int context_id)
{
	const char *FUNCNAME = "[GseAtmAal5RohcCtx::flush]";
	NetBurst *gse_packets;

	// flush corresponding GSE context
	gse_packets = GseCtx::flush(context_id);
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

NetBurst *GseAtmAal5RohcCtx::flushAll()
{
	const char *FUNCNAME = "[GseAtmAal5RohcCtx::flushAll]";
	NetBurst *gse_packets;

	// flush all GSE contexts
	gse_packets = this->GseAtmAal5Ctx::flushAll();
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

