/**
 * @file MpegAtmAal5RohcCtx.cpp
 * @brief MPEG2-TS/ATM/AAL5/ROHC encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "MpegAtmAal5RohcCtx.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"


MpegAtmAal5RohcCtx::MpegAtmAal5RohcCtx(unsigned long packing_threshold):
	RohcCtx(),
	MpegAtmAal5Ctx(packing_threshold)
{
}

MpegAtmAal5RohcCtx::~MpegAtmAal5RohcCtx()
{
}

NetBurst *MpegAtmAal5RohcCtx::encapsulate(NetPacket *packet,
                                          int &context_id,
                                          long &time)
{
	const char *FUNCNAME = "[MpegAtmAal5RohcCtx::encapsulate]";
	NetBurst *rohc_packets;
	NetBurst::iterator it;
	NetBurst *all_mpeg_packets;
	NetBurst *mpeg_packets;

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s packet is not valid, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	// ROHC encapsulation
	rohc_packets = RohcCtx::encapsulate(packet, context_id, time);
	if(rohc_packets == NULL)
	{
		UTI_ERROR("%s ROHC encapsulation failed, "
		          "drop packet\n", FUNCNAME);
		goto drop;
	}

	// create an empty burst of MPEG packets
	all_mpeg_packets = new NetBurst();
	if(all_mpeg_packets == NULL)
	{
		UTI_ERROR("%s cannot create a burst of packets, "
		          "drop packet\n", FUNCNAME);
		goto clean_rohc;
	}

	// MPEG2-TS/ATM/AAL5 encapsulation
	for(it = rohc_packets->begin(); it != rohc_packets->end(); it++)
	{
		mpeg_packets = MpegAtmAal5Ctx::encapsulate(*it, context_id, time);
		if(mpeg_packets == NULL)
		{
			UTI_ERROR("%s MPEG/ATM/AAL5 encapsulation failed, "
			          "drop packet\n", FUNCNAME);
			continue;
		}

		all_mpeg_packets->splice(all_mpeg_packets->end(), *mpeg_packets);

		// clean temporary burst of MPEG packets
		mpeg_packets->clear();
		delete mpeg_packets;
	}

	// clean temporary burst of ROHC packets
	delete rohc_packets;

	UTI_DEBUG("%s MPEG2-TS/ATM/AAL5/ROHC encapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 %s packet/frame => %d MPEG/ATM/AAL5/ROHC frames\n", FUNCNAME,
	          packet->name().c_str(), all_mpeg_packets->length());

	return all_mpeg_packets;

clean_rohc:
	delete rohc_packets;
drop:
	return NULL;
}

NetBurst *MpegAtmAal5RohcCtx::desencapsulate(NetPacket *packet)
{
	const char *FUNCNAME = "[MpegAtmAal5RohcCtx::desencapsulate]";
	NetBurst *rohc_packets;
	NetBurst::iterator it;
	NetBurst *all_net_packets;
	NetBurst *net_packets;

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s encapsulation packet is not valid, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	// packet must be a MPEG packet
	if(packet->type() != NET_PROTO_MPEG)
	{
		UTI_ERROR("%s encapsulation packet is not a MPEG packet, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	// MPEG2-TS/ATM/AAL5 desencapsulation
	rohc_packets = MpegAtmAal5Ctx::desencapsulate(packet);
	if(rohc_packets == NULL)
	{
		UTI_ERROR("%s MPEG/ATM/AAL5 desencapsulation failed, "
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
			UTI_ERROR("%s ROHC desencapsulation failed, "
			          "drop packet\n", FUNCNAME);
			continue;
		}

		all_net_packets->splice(all_net_packets->end(), *net_packets);

		// clean temporary burst of network packets extracted from ROHC packets
		net_packets->clear();
		delete net_packets;
	}

	// clean temporary burst of ROHC packets
	delete rohc_packets;

	UTI_DEBUG("%s MPEG2-TS/ATM/AAL5/ROHC desencapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 MPEG frame => %d %s packets/frames\n", FUNCNAME,
	          all_net_packets->length(), all_net_packets->name().c_str());

	return all_net_packets;

clean_rohc:
	delete rohc_packets;
drop:
	return NULL;
}

std::string MpegAtmAal5RohcCtx::type()
{
	return std::string("MPEG2-TS/ATM/AAL5/ROHC");
}

NetBurst *MpegAtmAal5RohcCtx::flush(int context_id)
{
	const char *FUNCNAME = "[MpegAtmAal5RohcCtx::flush]";
	NetBurst *mpeg_packets;

	// flush corresponding MPEG context
	mpeg_packets = MpegAtmAal5Ctx::flush(context_id);
	if(mpeg_packets == NULL)
	{
		UTI_ERROR("%s flushing MPEG context %d failed\n",
		          FUNCNAME, context_id);
		goto error;
	}

	UTI_DEBUG("%s %d MPEG frames flushed from context %d\n",
	          FUNCNAME, mpeg_packets->length(), context_id);

	return mpeg_packets;

error:
	return NULL;
}

NetBurst *MpegAtmAal5RohcCtx::flushAll()
{
	const char *FUNCNAME = "[MpegAtmAal5RohcCtx::flushAll]";
	NetBurst *mpeg_packets;

	// flush all MPEG contexts
	mpeg_packets = this->MpegAtmAal5Ctx::flushAll();
	if(mpeg_packets == NULL)
	{
		UTI_ERROR("%s flushing all MPEG contexts failed\n", FUNCNAME);
		goto error;
	}

	UTI_DEBUG("%s %d MPEG frames flushed from MPEG contexts\n",
	          FUNCNAME, mpeg_packets->length());

	return mpeg_packets;

error:
	return NULL;
}

