/**
 * @file GseAtmAal5Ctx.cpp
 * @brief GSE/ATM/AAL5 encapsulation / desencapsulation context
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#include "GseAtmAal5Ctx.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"


GseAtmAal5Ctx::GseAtmAal5Ctx(int qos_nbr, unsigned int packing_threshold):
  AtmAal5Ctx(),
  GseCtx(qos_nbr, packing_threshold, AtmCell::length())
{
}

GseAtmAal5Ctx::~GseAtmAal5Ctx()
{
}

NetBurst *GseAtmAal5Ctx::encapsulate(NetPacket *packet,
                                     int &context_id,
                                     long &time)
{
	const char *FUNCNAME = "[GseAtmAal5Ctx::encapsulate]";
	NetBurst *atm_cells;
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

	// ATM/AAL5 encapsulation
	atm_cells = AtmAal5Ctx::encapsulate(packet, context_id, time);
	if(atm_cells == NULL)
	{
		UTI_ERROR("%s ATM/AAL5 encapsulation failed, drop packet\n",
		          FUNCNAME);
		goto drop;
	}

	// create an empty burst of GSE packets
	all_gse_packets = new NetBurst();
	if(all_gse_packets == NULL)
	{
		UTI_ERROR("%s cannot create a burst of packets, drop packet\n",
		          FUNCNAME);
		goto clean_atm;
	}

	// GSE encapsulation
	for(it = atm_cells->begin(); it != atm_cells->end(); it++)
	{
		gse_packets = GseCtx::encapsulate(*it, context_id, time);
		if(gse_packets == NULL)
		{
			UTI_ERROR("%s GSE encapsulation failed, drop packet\n", FUNCNAME);
			continue;
		}

		all_gse_packets->splice(all_gse_packets->end(), *gse_packets);

		// clean temporary burst of GSE packets
		gse_packets->clear();
		delete gse_packets;
	}

	// clean temporary burst of ATM cells
	delete atm_cells;

	UTI_DEBUG("%s GSE/ATM/AAL5 encapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 %s packet/frame => %d GSE/ATM/AAL5 frames\n", FUNCNAME,
	          packet->name().c_str(), all_gse_packets->length());

	return all_gse_packets;

clean_atm:
	delete atm_cells;
drop:
	return NULL;
}

NetBurst * GseAtmAal5Ctx::desencapsulate(NetPacket *packet)
{
	const char *FUNCNAME = "[GseAtmAal5Ctx::desencapsulate]";
	NetBurst *atm_cells;
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

	// GSE desencapsulation
	atm_cells = GseCtx::desencapsulate(packet);
	if(atm_cells == NULL)
	{
		UTI_ERROR("%s GSE desencapsulation failed, drop packet\n", FUNCNAME);
		goto drop;
	}

	// create an empty burst of network packets
	all_net_packets = new NetBurst();
	if(all_net_packets == NULL)
	{
		UTI_ERROR("%s cannot create a burst of packets, drop packet\n", FUNCNAME);
		goto clean_atm;
	}

	// ATM/AAL5 desencapsulation
	for(it = atm_cells->begin(); it != atm_cells->end(); it++)
	{
		net_packets = AtmAal5Ctx::desencapsulate(*it);
		if(net_packets == NULL)
		{
			UTI_ERROR("%s ATM/AAL5 desencapsulation failed, drop packet\n",
			          FUNCNAME);
			continue;
		}

		all_net_packets->splice(all_net_packets->end(), *net_packets);

		// clean temporary burst of network packets extracted from ATM cells
		net_packets->clear();
		delete net_packets;
	}

	// clean temporary burst of ATM cells
	delete atm_cells;

	UTI_DEBUG("%s GSE/ATM/AAL5 desencapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 GSE frame => %d %s packets/frames\n", FUNCNAME,
	          all_net_packets->length(), all_net_packets->name().c_str());

	return all_net_packets;

clean_atm:
	delete atm_cells;
drop:
	return NULL;
}

std::string GseAtmAal5Ctx::type()
{
	return std::string("GSE/ATM/AAL5");
}

NetBurst *GseAtmAal5Ctx::flush(int context_id)
{
	const char *FUNCNAME = "[GseAtmAal5Ctx::flush]";
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

NetBurst *GseAtmAal5Ctx::flushAll()
{
	const char *FUNCNAME = "[GseAtmAal5Ctx::flushAll]";
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

