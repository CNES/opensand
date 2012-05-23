/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
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
 * @file Atm.h
 * @brief ATM encapsulation plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#ifndef ATM_CONTEXT_H
#define ATM_CONTEXT_H

#include <EncapPlugin.h>
#include <NetPacket.h>
#include <NetBurst.h>

#include <vector>
#include <map>

#include "AtmIdentifier.h"
#include "Aal5Packet.h"

/**
 * @class Atm
 * @brief ATM encapsulation plugin implementation
 */
class Atm: public EncapPlugin
{
  public:
	  
	/**
	 * @class Context
	 * @brief ATM encapsulation / desencapsulation context
	 */
	class Context: public EncapContext
	{
	  private:

		/// Temporary buffers for encapsulation contexts. Contexts are identified
		/// by an unique identifier
		std::map <AtmIdentifier *, Data *, ltAtmIdentifier> contexts;

	  public:

		/// constructor
		Context(EncapPlugin &plugin);

		/**
		 * Destroy the ATM encapsulation / deencapsulation context
		 */
		~Context();

		NetBurst *encapsulate(NetBurst *burst, std::map<long, int> &(time_contexts));
		NetBurst *deencapsulate(NetBurst *burst);
		NetBurst *flush(int UNUSED(context_id)) {return NULL;};
		NetBurst *flushAll() {return NULL;};

	  private:
		bool encapAtm(Aal5Packet *packet, NetBurst *atm_cells);
		NetBurst *deencapAtm(NetPacket *packet);
		Aal5Packet *encapAal5(NetPacket *packet);
		bool deencapAal5(NetBurst *aal5_packets, NetBurst *net_packets);
	};

	/**
	 * @class Packet
	 * @brief ATM packet
	 */
	class PacketHandler: public EncapPacketHandler
	{
	  public:

		PacketHandler(EncapPlugin &plugin);

		NetPacket *build(unsigned char *data, size_t data_length,
		                 uint8_t qos, uint8_t src_tal_id, uint8_t dst_tal_id);
		size_t getFixedLength() {return 53;};
		size_t getLength(const unsigned char *UNUSED(data))
		{
			return this->getFixedLength();
		};
		size_t getMinLength()
		{
			return this->getFixedLength();
		};
		bool getChunk(NetPacket *packet, size_t remaining_length,
		              NetPacket **data, NetPacket **remaining_data);
	};

	/// Constructor
	Atm();
};


CREATE(Atm, Atm::Context, Atm::PacketHandler);

#endif

