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
 * @file MpegAtmAal5RohcCtx.h
 * @brief MPEG2-TS/ATM/AAL5/ROHC encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef MPEG_ATM_AAL5_ROHC_CTX_H
#define MPEG_ATM_AAL5_ROHC_CTX_H

#include <map>
#include <string>

#include <EncapCtx.h>
#include <RohcCtx.h>
#include <MpegAtmAal5Ctx.h>
#include <NetPacket.h>
#include <MpegPacket.h>
#include <NetBurst.h>


/**
 * @class MpegAtmAal5RohcCtx
 * @brief MPEG2-TS/ATM/AAL5/ROHC encapsulation / desencapsulation context
 */
class MpegAtmAal5RohcCtx: public RohcCtx, MpegAtmAal5Ctx
{
 public:

	/**
	 * Build a MPEG2-TS/ATM/AAL5/ROHC encapsulation / desencapsulation context
	 *
	 * @param packing_threshold The Packing Threshold, ie. the maximum time (in
	 *                          milliseconds) to wait before sending an
	 *                          incomplete MPEG packet
	 */
	MpegAtmAal5RohcCtx(unsigned long packing_threshold);

	/**
	 * Destroy the MPEG2-TS/ATM/AAL5/ROHC encapsulation / desencapsulation context
	 */
	~MpegAtmAal5RohcCtx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);

	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif
