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
 * @file GseAtmAal5Ctx.h
 * @brief GSE/ATM/AAL5 encapsulation / desencapsulation context
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#ifndef GSE_ATM_AAL5_CTX_H
#define GSE_ATM_AAL5_CTX_H

#include <string>

#include <EncapCtx.h>
#include <GseCtx.h>
#include <AtmAal5Ctx.h>
#include <NetPacket.h>
#include <GsePacket.h>
#include <AtmCell.h>
#include <NetBurst.h>


/**
 * @class GseAtmAal5Ctx
 * @brief GSE/ATM/AAL5 encapsulation / desencapsulation context
 */
class GseAtmAal5Ctx: public AtmAal5Ctx, public GseCtx
{
 public:

	/**
	 * Build a GSE/ATM/AAL5 encapsulation / desencapsulation context
	 *
	 * @param qos_nbr            The number of QoS possible values used
	 *                           for GSE Frag ID
	 * @param packing_threshold  The maximum time (ms) to wait before sending
	 *                           an incomplete MPEG packet
	 */
	GseAtmAal5Ctx(int qos_nbr, unsigned int packing_threshold);

	/**
	 * Destroy the GSE/ATM/AAL5 encapsulation / desencapsulation context
	 */
	~GseAtmAal5Ctx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);

	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif
