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
 * @file AtmAal5Ctx.h
 * @brief ATM/AAL5 encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ATM_AAL5_CTX_H
#define ATM_AAL5_CTX_H

#include <map>
#include <string>

#include <EncapCtx.h>
#include <Aal5Ctx.h>
#include <AtmCtx.h>
#include <NetPacket.h>
#include <NetBurst.h>


/**
 * @class AtmAal5Ctx
 * @brief ATM/AAL5 encapsulation / desencapsulation context
 */
class AtmAal5Ctx: public Aal5Ctx, public AtmCtx
{
 public:

	/**
	 * Build a ATM/AAL5 encapsulation / desencapsulation context
	 */
	AtmAal5Ctx();

	/**
	 * Destroy the ATM/AAL5 encapsulation / desencapsulation context
	 */
	~AtmAal5Ctx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);

	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif
