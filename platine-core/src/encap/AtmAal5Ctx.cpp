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
 * @file AtmAal5Ctx.cpp
 * @brief ATM/AAL5 encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "AtmAal5Ctx.h"

#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"


AtmAal5Ctx::AtmAal5Ctx(): Aal5Ctx(), AtmCtx()
{
}

AtmAal5Ctx::~AtmAal5Ctx()
{
}

NetBurst *AtmAal5Ctx::encapsulate(NetPacket *packet,
                                   int &context_id,
                                   long &time)
{
	const char *FUNCNAME = "[AtmAal5Ctx::encapsulate]";
	NetBurst *aal5_packets;
	NetBurst *atm_cells;

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s packet is not valid, drop the packet\n", FUNCNAME);
		goto drop;
	}

	// AAL5 encapsulation
	aal5_packets = this->Aal5Ctx::encapsulate(packet, context_id, time);
	if(aal5_packets == NULL)
	{
		UTI_ERROR("%s AAL5 encapsulation failed, drop packet\n", FUNCNAME);
		goto drop;
	}

	// AAL5 encapsulation should not produce more than one AAL5 packet
	if(aal5_packets->length() != 1)
	{
		UTI_ERROR("%s AAL5 encapsulation produced too few or too many "
		          "AAL5 packets (%d), drop packet\n", FUNCNAME,
		          aal5_packets->length());
		goto clean_aal5;
	}

	// ATM encapsulation
	atm_cells = this->AtmCtx::encapsulate(aal5_packets->front(),
	                                      context_id, time);
	if(atm_cells == NULL)
	{
		UTI_ERROR("%s ATM encapsulation failed, drop packet\n", FUNCNAME);
		goto clean_aal5;
	}

	// clean temporary burst of AAL5 packets
	delete aal5_packets;

	UTI_DEBUG("%s ATM/AAL5 encapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 %s packet/frame => %d ATM/AAL5 frames\n", FUNCNAME,
	          packet->name().c_str(), atm_cells->length());

	return atm_cells;

clean_aal5:
	delete aal5_packets;
drop:
	return NULL;
}

NetBurst *AtmAal5Ctx::desencapsulate(NetPacket *packet)
{
	const char *FUNCNAME = "[AtmAal5Ctx::desencapsulate]";
	NetBurst *aal5_packets;
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

	// packet must be an ATM cell
	if(packet->type() != NET_PROTO_ATM)
	{
		UTI_ERROR("%s encapsulation packet is not an ATM cell, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	// ATM desencapsulation
	aal5_packets = this->AtmCtx::desencapsulate(packet);
	if(aal5_packets == NULL)
	{
		UTI_ERROR("%s ATM desencapsulation failed, drop packet\n", FUNCNAME);
		goto drop;
	}

	// create an empty burst of network packets
	all_net_packets = new NetBurst();
	if(all_net_packets == NULL)
	{
		UTI_ERROR("%s cannot create a burst of packets, drop packet\n", FUNCNAME);
		goto clean_aal5;
	}

	// AAL5 desencapsulation
	for(it = aal5_packets->begin(); it != aal5_packets->end(); it++)
	{
		net_packets = this->Aal5Ctx::desencapsulate(*it);
		if(net_packets == NULL)
		{
			UTI_ERROR("%s AAL5 desencapsulation failed, drop packet\n", FUNCNAME);
			continue;
		}

		// AAL5 desencapsulation should not produce more than one network packet
		if(net_packets->length() != 1)
		{
			UTI_ERROR("%s AAL5 desencapsulation produced too many network packets "
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

	// clean temporary burst of AAL5 packets
	delete aal5_packets;

	UTI_DEBUG("%s ATM/AAL5 desencapsulation finished\n", FUNCNAME);
	UTI_DEBUG("%s 1 ATM cell => %d %s packets/frames\n", FUNCNAME,
	          all_net_packets->length(), all_net_packets->name().c_str());

	return all_net_packets;

clean_aal5:
	delete aal5_packets;
drop:
	return NULL;
}

std::string AtmAal5Ctx::type()
{
	return std::string("ATM/AAL5");
}

NetBurst *AtmAal5Ctx::flush(int context_it)
{
	// nothing to do for ATM/AAL5
	UTI_DEBUG("[AtmAal5Ctx::flush] do nothing\n");
	return NULL;
}

NetBurst *AtmAal5Ctx::flushAll()
{
	// nothing to do for ATM/AAL5
	UTI_DEBUG("[AtmAal5Ctx::flushAll] do nothing\n");
	return NULL;
}
