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
 * @file GseMpegUleCtx.cpp
 * @brief GSE/MPEG/ULE encapsulation / desencapsulation context
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#include "GseMpegUleCtx.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"


GseMpegUleCtx::GseMpegUleCtx(int qos_nbr, unsigned int packing_threshold):
  MpegUleCtx(packing_threshold),
  GseCtx(qos_nbr, packing_threshold, MpegPacket::length())
{
}

GseMpegUleCtx::~GseMpegUleCtx()
{
}

NetBurst *GseMpegUleCtx::encapsulate(NetPacket *packet,
                                     int &context_id,
                                     long &time)
{
	const char *FUNCNAME = "[GseMpegUleCtx::encapsulate]";
	NetBurst *mpeg_packets;
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

	// MPEG/ULE encapsulation
	mpeg_packets = MpegUleCtx::encapsulate(packet, context_id, time);
	if(mpeg_packets == NULL)
	{
		UTI_ERROR("%s MPEG/ULE encapsulation failed, drop packet\n",
		          FUNCNAME);
		goto drop;
	}

	// create an empty burst of GSE packets
	all_gse_packets = new NetBurst();
	if(all_gse_packets == NULL)
	{
		UTI_ERROR("%s cannot create a burst of packets, drop packet\n",
		          FUNCNAME);
		goto clean_mpeg;
	}

	// GSE encapsulation
	for(it = mpeg_packets->begin(); it != mpeg_packets->end(); it++)
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

	// clean temporary burst of MPEG packets
	delete mpeg_packets;

	UTI_DEBUG("%s GSE/MPEG/ULE encapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 %s packet/frame => %d GSE/MPEG/ULE frames\n", FUNCNAME,
	          packet->name().c_str(), all_gse_packets->length());

	return all_gse_packets;

clean_mpeg:
	delete mpeg_packets;
drop:
	return NULL;
}

NetBurst *GseMpegUleCtx::desencapsulate(NetPacket *packet)
{
	const char *FUNCNAME = "[GseMpegUleCtx::desencapsulate]";
	NetBurst *mpeg_packets;
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

	// packet must be a GSE packet
	if(packet->type() != NET_PROTO_GSE)
	{
		UTI_ERROR("%s encapsulation packet is not a GSE packet, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	// GSE desencapsulation
	mpeg_packets = GseCtx::desencapsulate(packet);
	if(mpeg_packets == NULL)
	{
		UTI_ERROR("%s GSE desencapsulation failed, "
		          "drop packet\n", FUNCNAME);
		goto drop;
	}

	// create an empty burst of network packets
	all_net_packets = new NetBurst();
	if(all_net_packets == NULL)
	{
		UTI_ERROR("%s cannot create a burst of packets, "
		          "drop packet\n", FUNCNAME);
		goto clean_mpeg;
	}

	// MPEG/ULE desencapsulation
	for(it = mpeg_packets->begin(); it != mpeg_packets->end(); it++)
	{
		net_packets = MpegUleCtx::desencapsulate(*it);
		if(net_packets == NULL)
		{
			UTI_ERROR("%s MPEG/ULE desencapsulation failed, "
			          "drop packet\n", FUNCNAME);
			continue;
		}

		all_net_packets->splice(all_net_packets->end(), *net_packets);

		// clean temporary burst of network packets extracted from MPEG packets
		net_packets->clear();
		delete net_packets;
	}

	// clean temporary burst of MPEG packets
	delete mpeg_packets;

	UTI_DEBUG("%s GSE/MPEG/ULE desencapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 GSE frame => %d %s packets/frames\n", FUNCNAME,
	          all_net_packets->length(), all_net_packets->name().c_str());

	return all_net_packets;

clean_mpeg:
	delete mpeg_packets;
drop:
	return NULL;
}

std::string GseMpegUleCtx::type()
{
	return std::string("GSE/MPEG/ULE");
}

NetBurst *GseMpegUleCtx::flush(int context_id)
{
	const char *FUNCNAME = "[GseMpegUleCtx::flush]";
	NetBurst *gse_packets;

	// flush corresponding GSE context
	gse_packets = GseCtx::flush(context_id);
	if(gse_packets == NULL)
	{
		UTI_ERROR("%s flushing GSE context %d failed\n",
		          FUNCNAME, context_id);
		goto error;
	}

	UTI_DEBUG("%s %d GSE frames flushed from context %d\n",
	          FUNCNAME, gse_packets->length(), context_id);

	return gse_packets;

error:
	return NULL;
}

NetBurst *GseMpegUleCtx::flushAll()
{
	const char *FUNCNAME = "[GseMpegUleCtx::flushAll]";
	NetBurst *gse_packets;

	// flush all GSE contexts
	gse_packets = this->GseCtx::flushAll();
	if(gse_packets == NULL)
	{
		UTI_ERROR("%s flushing all GSE contexts failed\n", FUNCNAME);
		goto error;
	}

	UTI_DEBUG("%s %d GSE frames flushed from GSE contexts\n",
	          FUNCNAME, gse_packets->length());

	return gse_packets;

error:
	return NULL;
}
