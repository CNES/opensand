/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
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
 * @file Ule.h
 * @brief ULE encapsulation plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#ifndef ULE_CONTEXT_H
#define ULE_CONTEXT_H

#include <EncapPlugin.h>
#include <NetPacket.h>
#include <NetBurst.h>

#include <vector>
#include <map>
#include <cassert>

#include "UleExt.h"

/**
 * @class Ule
 * @brief ULE encapsulation plugin implementation
 */
class Ule: public EncapPlugin
{
  public:

	/**
	 * @class Context
	 * @brief ULE encapsulation / desencapsulation context
	 */
	class Context: public EncapContext
	{
	  private:

		/// List of handlers for mandatory ULE extensions
		std::map < uint8_t, UleExt * > mandatory_exts;

		/// List of handlers for optional ULE extensions
		std::map < uint8_t, UleExt * > optional_exts;

		/// List of extension handlers to use when building ULE packets
		std::list < UleExt * > build_exts;

		/// do we enable CRC computing ?
		bool enable_crc;

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

	  public:

		/// constructor
		Context(EncapPlugin &plugin);

		/**
		 * Destroy the ULE encapsulation / deencapsulation context
		 */
		~Context();

		NetBurst *encapsulate(NetBurst *burst, std::map<long, int> &time_contexts);
		NetBurst *deencapsulate(NetBurst *burst);
		NetBurst *flush(int UNUSED(context_id)) {return NULL;};
		NetBurst *flushAll() {return NULL;};

	  private:
		bool encapUle(NetPacket *packet, NetBurst *ule_packets);
		bool deencapUle(NetPacket *packet, NetBurst *net_packets);
	};

	/**
	 * @class Packet
	 * @brief ULE packet
	 */
	class PacketHandler: public EncapPacketHandler
	{
	  public:

		PacketHandler(EncapPlugin &plugin);

		NetPacket *build(const Data &data,
		                 size_t data_length,
		                 uint8_t qos,
		                 uint8_t src_tal_id,
		                 uint8_t dst_tal_id) const;
		size_t getFixedLength() const {return 0;};
		size_t getMinLength() const {return 2;};
		size_t getLength(const unsigned char *data) const;
		bool getChunk(NetPacket *UNUSED(packet),
		              size_t UNUSED(remaining_length),
		              NetPacket **UNUSED(data),
		              NetPacket **UNUSED(remaining_data)) const
		{
			assert(0);
		};
		bool getSrc(const Data &UNUSED(data), tal_id_t &UNUSED(tal_id)) const
		{
			assert(0);
		};
	};

	/// Constructor
	Ule();
};


CREATE(Ule, Ule::Context, Ule::PacketHandler, "ULE");

#endif

