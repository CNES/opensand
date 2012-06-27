/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
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
 * @file Rohc.h
 * @brief ROHC encapsulation plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#ifndef ROHC_CONTEXT_H
#define ROHC_CONTEXT_H

#include <EncapPlugin.h>
#include <NetPacket.h>
#include <NetBurst.h>
#include <cassert>

#include <vector>
#include <map>


#include <RohcPacket.h>

extern "C"
{
	#include <rohc.h>
	#include <rohc_comp.h>
	#include <rohc_decomp.h>
}

#define MAX_ROHC_SIZE      (5 * 1024)

/**
 * @class Rohc
 * @brief ROHC compression plugin implementation
 */
class Rohc: public EncapPlugin
{
  public:
	  
	/**
	 * @class Context
	 * @brief ROHC compression / decompression context
	 */
	class Context: public EncapContext
	{
	  private:

		/// The ROHC compressor
		struct rohc_comp *comp;
		/// The ROHC decompressor
		//struct rohc_decomp *decomp;

	  public:

		/// constructor
		Context(EncapPlugin &plugin);

		/**
		 * Destroy the ROHC compression / decompression context
		 */
		~Context();

		NetBurst *encapsulate(NetBurst *burst, std::map<long, int> &time_contexts);
		NetBurst *deencapsulate(NetBurst *burst);
		NetBurst *flush(int UNUSED(context_id)) {return NULL;};
		NetBurst *flushAll() {return NULL;};

	  private:

		bool compressRohc(NetPacket *packet, NetBurst *rohc_packets);
		bool decompressRohc(NetPacket *packet, NetBurst *net_packets);

		std::map<uint8_t, struct rohc_decomp*> decompressors;

	};

	/**
	 * @class Packet
	 * @brief ROHC packet
	 */
	class PacketHandler: public EncapPacketHandler
	{

	  public:

		PacketHandler(EncapPlugin &plugin);

		NetPacket *build(unsigned char *data, size_t data_length,
		                 uint8_t qos, uint8_t src_tal_id, uint8_t dst_tal_id);
		size_t getFixedLength() {return 0;};
		size_t getLength(const unsigned char *UNUSED(data)) {return 0;};
		size_t getMinLength() {assert(0);};
		bool getChunk(NetPacket *UNUSED(packet),
		              size_t UNUSED(remaining_length),
		              NetPacket **UNUSED(data),
		              NetPacket **UNUSED(remaining_data))
		{
			assert(0);
		};
	};

	/// Constructor
	Rohc();
};

CREATE(Rohc, Rohc::Context, Rohc::PacketHandler);

#endif

