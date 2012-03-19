/**
 * @file MpegUleCtx.cpp
 * @brief MPEG2-TS/ULE encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "MpegUleCtx.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"


MpegUleCtx::MpegUleCtx(unsigned long packing_threshold):
	UleCtx(),
	MpegCtx(2, packing_threshold,
	        UlePacket::length, UlePacket::create)
{
}

MpegUleCtx::~MpegUleCtx()
{
}

NetBurst *MpegUleCtx::encapsulate(NetPacket *packet,
                                  int &context_id,
                                  long &time)
{
	const char *FUNCNAME = "[MpegUleCtx::encapsulate]";
	NetBurst *ule_packets;
	NetBurst *mpeg_packets;

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s packet is not valid, drop the packet\n", FUNCNAME);
		goto drop;
	}

	// ULE encapsulation
	ule_packets = this->UleCtx::encapsulate(packet, context_id, time);
	if(ule_packets == NULL)
	{
		UTI_ERROR("%s ULE encapsulation failed, drop packet\n", FUNCNAME);
		goto drop;
	}

	// ULE encapsulation should not produce more than one ULE packet
	if(ule_packets->length() != 1)
	{
		UTI_ERROR("%s ULE encapsulation produced too few or too many ULE packets "
		          "(%d), drop packet\n", FUNCNAME, ule_packets->length());
		goto clean_ule;
	}

	// MPEG2-TS encapsulation
	mpeg_packets = this->MpegCtx::encapsulate(ule_packets->front(), context_id, time);
	if(mpeg_packets == NULL)
	{
		UTI_ERROR("%s MPEG encapsulation failed, drop packet\n", FUNCNAME);
		goto clean_ule;
	}

	// clean temporary burst of ULE packets
	delete ule_packets;

	UTI_DEBUG("%s MPEG2-TS/ULE encapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 %s packet/frame => %d MPEG/ULE frames\n", FUNCNAME,
	          packet->name().c_str(), mpeg_packets->length());

	return mpeg_packets;

clean_ule:
	delete ule_packets;
drop:
	return NULL;
}

NetBurst *MpegUleCtx::desencapsulate(NetPacket *packet)
{
	const char *FUNCNAME = "[MpegUleCtx::desencapsulate]";
	NetBurst *ule_packets;
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

	// MPEG2-TS desencapsulation
	ule_packets = this->MpegCtx::desencapsulate(packet);
	if(ule_packets == NULL)
	{
		UTI_ERROR("%s MPEG desencapsulation failed, drop packet\n", FUNCNAME);
		goto drop;
	}

	// create an empty burst of network packets
	all_net_packets = new NetBurst();
	if(all_net_packets == NULL)
	{
		UTI_ERROR("%s cannot create a burst of packets, drop packet\n", FUNCNAME);
		goto clean_ule;
	}

	// ULE desencapsulation
	for(it = ule_packets->begin(); it != ule_packets->end(); it++)
	{
		net_packets = this->UleCtx::desencapsulate(*it);
		if(net_packets == NULL)
		{
			UTI_ERROR("%s ULE desencapsulation failed, drop packet\n", FUNCNAME);
			continue;
		}

		// ULE desencapsulation should not produce more than one network packet
		if(net_packets->length() != 1)
		{
			UTI_ERROR("%s ULE desencapsulation produced too many network packets "
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

	// clean temporary burst of ULE packets
	delete ule_packets;

	UTI_DEBUG("%s MPEG2-TS/ULE desencapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 MPEG frame => %d %s packets/frames\n", FUNCNAME,
	          all_net_packets->length(), all_net_packets->name().c_str());

	return all_net_packets;

clean_ule:
	delete ule_packets;
drop:
	return NULL;
}

std::string MpegUleCtx::type()
{
	return std::string("MPEG2-TS/ULE");
}

NetBurst *MpegUleCtx::flush(int context_id)
{
	const char *FUNCNAME = "[MpegUleCtx::flush]";
	NetBurst *mpeg_packets;

	// flush corresponding MPEG context
	mpeg_packets = this->MpegCtx::flush(context_id);
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

NetBurst * MpegUleCtx::flushAll()
{
	const char *FUNCNAME = "[MpegUleCtx::flushAll]";
	NetBurst *mpeg_packets;

	// flush all MPEG contexts
	mpeg_packets = this->MpegCtx::flushAll();
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

