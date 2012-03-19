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
 * @file MpegAtmAal5Ctx.cpp
 * @brief MPEG2-TS/ATM/AAL5 encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "MpegAtmAal5Ctx.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"


MpegAtmAal5Ctx::MpegAtmAal5Ctx(unsigned long packing_threshold):
	AtmAal5Ctx(),
	MpegCtx(AtmCell::length(), packing_threshold,
	        AtmCell::length, AtmCell::create)
{
}

MpegAtmAal5Ctx::~MpegAtmAal5Ctx()
{
}

NetBurst *MpegAtmAal5Ctx::encapsulate(NetPacket *packet,
                                      int &context_id,
                                      long &time)
{
	const char *FUNCNAME = "[MpegAtmAal5Ctx::encapsulate]";
	NetBurst *atm_cells;
	NetBurst::iterator it;
	NetBurst *all_mpeg_packets;
	NetBurst *mpeg_packets;

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
		UTI_ERROR("%s ATM/AAL5 encapsulation failed, "
		          "drop packet\n", FUNCNAME);
		goto drop;
	}

	// create an empty burst of MPEG packets
	all_mpeg_packets = new NetBurst();
	if(all_mpeg_packets == NULL)
	{
		UTI_ERROR("%s cannot create a burst of packets, "
		          "drop packet\n", FUNCNAME);
		goto clean_atm;
	}

	// MPEG2-TS encapsulation
	for(it = atm_cells->begin(); it != atm_cells->end(); it++)
	{
		mpeg_packets = MpegCtx::encapsulate(*it, context_id, time);
		if(mpeg_packets == NULL)
		{
			UTI_ERROR("%s MPEG encapsulation failed, "
			          "drop packet\n", FUNCNAME);
			continue;
		}

		all_mpeg_packets->splice(all_mpeg_packets->end(), *mpeg_packets);

		// clean temporary burst of MPEG packets
		mpeg_packets->clear();
		delete mpeg_packets;
	}

	// clean temporary burst of ATM cells
	delete atm_cells;

	UTI_DEBUG("%s MPEG2-TS/ATM/AAL5 encapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 %s packet/frame => %d MPEG/ATM/AAL5 frames\n", FUNCNAME,
	          packet->name().c_str(), all_mpeg_packets->length());

	return all_mpeg_packets;

clean_atm:
	delete atm_cells;
drop:
	return NULL;
}

NetBurst *MpegAtmAal5Ctx::desencapsulate(NetPacket *packet)
{
	const char *FUNCNAME = "[MpegAtmAal5Ctx::desencapsulate]";
	NetBurst *atm_cells;
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

	// MPEG2-TS desencapsulation
	atm_cells = MpegCtx::desencapsulate(packet);
	if(atm_cells == NULL)
	{
		UTI_ERROR("%s MPEG desencapsulation failed, "
		          "drop packet\n", FUNCNAME);
		goto drop;
	}

	// create an empty burst of network packets
	all_net_packets = new NetBurst();
	if(all_net_packets == NULL)
	{
		UTI_ERROR("%s cannot create a burst of packets, "
		          "drop packet\n", FUNCNAME);
		goto clean_atm;
	}

	// ATM/AAL5 desencapsulation
	for(it = atm_cells->begin(); it != atm_cells->end(); it++)
	{
		net_packets = AtmAal5Ctx::desencapsulate(*it);
		if(net_packets == NULL)
		{
			UTI_ERROR("%s ATM/AAL5 desencapsulation failed, "
			          "drop packet\n", FUNCNAME);
			continue;
		}

		all_net_packets->splice(all_net_packets->end(), *net_packets);

		// clean temporary burst of network packets extracted from ATM cells
		net_packets->clear();
		delete net_packets;
	}

	// clean temporary burst of ATM cells
	delete atm_cells;

	UTI_DEBUG("%s MPEG2-TS/ATM/AAL5 desencapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 MPEG frame => %d %s packets/frames\n", FUNCNAME,
	          all_net_packets->length(), all_net_packets->name().c_str());

	return all_net_packets;

clean_atm:
	delete atm_cells;
drop:
	return NULL;
}

std::string MpegAtmAal5Ctx::type()
{
	return std::string("MPEG2-TS/ATM/AAL5");
}

NetBurst *MpegAtmAal5Ctx::flush(int context_id)
{
	const char *FUNCNAME = "[MpegAtmAal5Ctx::flush]";
	NetBurst *mpeg_packets;

	// flush corresponding MPEG context
	mpeg_packets = MpegCtx::flush(context_id);
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

NetBurst * MpegAtmAal5Ctx::flushAll()
{
	const char *FUNCNAME = "[MpegAtmAal5Ctx::flushAll]";
	NetBurst *mpeg_packets;

	// flush all MPEG contexts
	mpeg_packets = this->MpegCtx::flushAll();
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
