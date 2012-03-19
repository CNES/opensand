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
 * @file UleCtx.h
 * @brief ULE encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ULE_CTX_H
#define ULE_CTX_H

#include <map>
#include <list>
#include <string>

#include <EncapCtx.h>
#include <UleExt.h>
#include <NetPacket.h>
#include <UlePacket.h>
#include <Ipv4Packet.h>
#include <Ipv6Packet.h>
#include <NetBurst.h>


/**
 * @class UleCtx
 * @brief ULE encapsulation / desencapsulation context
 */
class UleCtx: public EncapCtx
{
 private:

	/// List of handlers for mandatory ULE extensions
	std::map < uint8_t, UleExt * > mandatory_exts;

	/// List of handlers for optional ULE extensions
	std::map < uint8_t, UleExt * > optional_exts;

	/// List of extension handlers to use when building ULE packets
	std::list < UleExt * > build_exts;

 public:

	/**
	 * Build a ULE encapsulation / desencapsulation context
	 */
	UleCtx();

	/**
	 * Destroy the ULE encapsulation / desencapsulation context
	 */
	~UleCtx();

	/**
	 * Add an extension handler to the ULE encapsulation context
	 *
	 * @param ext        The extension handler to add
	 * @param activated  Whether the extension handler must be use to build ULE
	 *                   packets or not
	 * @return           Whether the extension handler was successfully added
	 *                   or not
	 */
	bool addExt(UleExt *ext, bool activated);

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);
	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif
