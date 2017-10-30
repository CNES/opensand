/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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
 * @file Rle.h
 * @brief RLE encapsulation plugin implementation
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef RLE_CONTEXT_H
#define RLE_CONTEXT_H

#include "RleIdentifier.h"

#include <EncapPlugin.h>
#include <NetPacket.h>
#include <NetBurst.h>

#include <vector>
#include <map>

extern "C"
{
	#include <rle.h>
}

typedef enum {
	rle_alpdu_crc,
	rle_alpdu_sequence_number
} rle_alpdu_protection_t;

/**
 * @class Rle
 * @brief RLEE encapsulation plugin implementation
 */
class Rle: public EncapPlugin
{
  public:

	/**
	 * @class Context
	 * @brief RLE encapsulation / desencapsulation context
	 */
	class Context: public EncapContext
	{
	  public:

		/// constructor
		Context(EncapPlugin &plugin);

		/**
		 * Destroy the GSE encapsulation / deencapsulation context
		 */
		~Context();

		NetBurst *encapsulate(NetBurst *burst, std::map<long, int> &time_contexts);
		NetBurst *deencapsulate(NetBurst *burst);
		NetBurst *flush(int context_id);
		NetBurst *flushAll();

	  private:
		/// RLE configuration
		struct rle_config rle_conf;

		// Receivers identified by an unique identifier
		std::map <RleIdentifier *, struct rle_receiver *, ltRleIdentifier> receivers;

		bool decapNextPacket(NetPacket *packet, NetBurst *burst);
	};

	/**
	 * @class Packet
	 * @brief RLE packet
	 */
	class PacketHandler: public EncapPacketHandler
	{
	  private:

		/// RLE configuration
		struct rle_config rle_conf;

		// Transmitters identified by an unique identifier
		std::map <RleIdentifier *, struct rle_transmitter *, ltRleIdentifier> transmitters;
		std::map <RleIdentifier *, NetPacket *, ltRleIdentifier> partial_sent;

	  public:

		PacketHandler(EncapPlugin &plugin);
		~PacketHandler();

		bool init();

		NetPacket *build(const Data &data,
		                 size_t data_length,
		                 uint8_t qos,
		                 uint8_t src_tal_id,
		                 uint8_t dst_tal_id) const;
		size_t getFixedLength() const {return 0;};
		size_t getMinLength() const {return 3;};
		size_t getLength(const unsigned char *data) const;
		bool getSrc(const Data &data, tal_id_t &tal_id) const;
		bool getQos(const Data &data, qos_t &qos) const;

		bool encapNextPacket(NetPacket *packet,
			size_t remaining_length,
			bool &partial_encap,
			NetPacket **encap_packet);
		bool resetPacketToEncap(NetPacket *packet = NULL);

	  protected:
		bool getChunk(NetPacket *packet, size_t remaining_length,
		              NetPacket **data, NetPacket **remaining_data) const;
		bool resetOnePacketToEncap(NetPacket *packet);
		bool resetAllPacketToEncap();
	};

	/// Constructor
	Rle();

	static bool getLabel(NetPacket *packet, uint8_t label[]);
	static bool getLabel(const Data &data, uint8_t label[]);

	static void rle_traces(const int module_id,
		const int level,
		const char *const file,
		const int line,
		const char *const func,
		const char *const message,
		...);
};

CREATE(Rle, Rle::Context, Rle::PacketHandler, "RLE");

#endif

