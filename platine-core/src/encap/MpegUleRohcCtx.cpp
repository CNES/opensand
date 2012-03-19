/**
 * @file MpegUleRohcCtx.cpp
 * @brief MPEG2-TS/ULE encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "MpegUleRohcCtx.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"


MpegUleRohcCtx::MpegUleRohcCtx(unsigned long packing_threshold):
	RohcCtx(),
	MpegUleCtx(packing_threshold)
{
}

MpegUleRohcCtx::~MpegUleRohcCtx()
{
}

NetBurst *MpegUleRohcCtx::encapsulate(NetPacket *packet,
                                      int &context_id,
                                      long &time)
{
	const char *FUNCNAME = "[MpegUleRohcCtx::encapsulate]";
	NetBurst *rohc_packets;
	NetBurst *mpeg_packets;

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
		UTI_ERROR("%s ROHC encapsulation produced too few or too many ROHC packets "
		          "(%d), drop packet\n", FUNCNAME, rohc_packets->length());
		goto clean_rohc;
	}

	// MPEG2-TS/ULE encapsulation
	mpeg_packets = this->MpegUleCtx::encapsulate(rohc_packets->front(), context_id, time);
	if(mpeg_packets == NULL)
	{
		UTI_ERROR("%s MPEG/ULE encapsulation failed, drop packet\n", FUNCNAME);
		goto clean_rohc;
	}

	// clean temporary burst of ROHC packets
	delete rohc_packets;

	UTI_DEBUG("%s MPEG2-TS/ULE/ROHC encapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 %s packet/frame => %d MPEG/ULE/ROHC frames\n", FUNCNAME,
	          packet->name().c_str(), mpeg_packets->length());

	return mpeg_packets;

clean_rohc:
	delete rohc_packets;
drop:
	return NULL;
}

NetBurst *MpegUleRohcCtx::desencapsulate(NetPacket *packet)
{
	const char *FUNCNAME = "[MpegUleRohcCtx::desencapsulate]";
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

	// packet must be a MPEG packet
	if(packet->type() != NET_PROTO_MPEG)
	{
		UTI_ERROR("%s encapsulation packet is not a MPEG packet, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	// MPEG2-TS/ULE desencapsulation
	rohc_packets = this->MpegUleCtx::desencapsulate(packet);
	if(rohc_packets == NULL)
	{
		UTI_ERROR("%s MPEG/ULE desencapsulation failed, drop packet\n", FUNCNAME);
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

	UTI_DEBUG("%s MPEG2-TS/ULE/ROHC desencapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 MPEG frame => %d %s packets/frames\n", FUNCNAME,
	          all_net_packets->length(), all_net_packets->name().c_str());

	return all_net_packets;

clean_rohc:
	delete rohc_packets;
drop:
	return NULL;
}

std::string MpegUleRohcCtx::type()
{
	return std::string("MPEG2-TS/ULE/ROHC");
}

NetBurst * MpegUleRohcCtx::flush(int context_id)
{
	const char *FUNCNAME = "[MpegUleRohcCtx::flush]";
	NetBurst *mpeg_packets;

	// flush corresponding MPEG context
	mpeg_packets = this->MpegUleCtx::flush(context_id);
	if(mpeg_packets == NULL)
	{
		UTI_ERROR("%s flushing MPEG context %d failed\n", FUNCNAME, context_id);
		goto error;
	}

	UTI_DEBUG("%s %d MPEG frames flushed from context %d\n", FUNCNAME,
	          mpeg_packets->length(), context_id);

	return mpeg_packets;

error:
	return NULL;
}

NetBurst *MpegUleRohcCtx::flushAll()
{
	const char *FUNCNAME = "[MpegUleRohcCtx::flushAll]";
	NetBurst *mpeg_packets;

	// flush all MPEG contexts
	mpeg_packets = this->MpegUleCtx::flushAll();
	if(mpeg_packets == NULL)
	{
		UTI_ERROR("%s flushing all MPEG contexts failed\n", FUNCNAME);
		goto error;
	}

	UTI_DEBUG("%s %d MPEG frames flushed from MPEG contexts\n", FUNCNAME,
	          mpeg_packets->length());

	return mpeg_packets;

error:
	return NULL;
}

