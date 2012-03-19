/*
 *
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
 * @file RohcCtx.h
 * @brief ROHC encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ROHC_CTX_H
#define ROHC_CTX_H

#include <EncapCtx.h>
#include <NetPacket.h>
#include <RohcPacket.h>
#include <Ipv4Packet.h>
#include <Ipv6Packet.h>
#include <NetBurst.h>

extern "C"
{
	#include <rohc.h>
	#include <rohc_comp.h>
	#include <rohc_decomp.h>
}

#define MAX_ROHC_SIZE      (5 * 1024)

/**
 * @class RohcCtx
 * @brief ROHC encapsulation / desencapsulation context
 */
class RohcCtx: public EncapCtx
{
 private:

	/// The ROHC compressor
	struct rohc_comp *comp;
	/// The ROHC decompressor
	struct rohc_decomp *decomp;

 public:

	/**
	 * Build a ROHC encapsulation / desencapsulation context
	 */
	RohcCtx();

	/**
	 * Destroy the ROHC encapsulation / desencapsulation context
	 */
	~RohcCtx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);
	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif
