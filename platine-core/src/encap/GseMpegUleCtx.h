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
 * @file GseMpegUleCtx.h
 * @brief GSE/MPEG/ULE encapsulation / desencapsulation context
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#ifndef GSE_MPEG_ULE_CTX_H
#define GSE_MPEG_ULE_CTX_H

#include <string>

#include <EncapCtx.h>
#include <GseCtx.h>
#include <MpegUleCtx.h>
#include <NetPacket.h>
#include <GsePacket.h>
#include <MpegPacket.h>
#include <NetBurst.h>


/**
 * @class GseMpegUleCtx
 * @brief GSE/MPEG/ULE encapsulation / desencapsulation context
 */
class GseMpegUleCtx: public MpegUleCtx, public GseCtx
{
 public:

	/**
	 * Build a GSE/MPEG/ULE encapsulation / desencapsulation context
	 *
	 * @param packing_threshold  The number of QoS possible values used
	 *                           for GSE Frag ID
	 */
	GseMpegUleCtx(int qos_nbr, unsigned int packing_threshold);

	/**
	 * Destroy the GSE/MPEG/ULE encapsulation / desencapsulation context
	 */
	~GseMpegUleCtx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);

	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif
