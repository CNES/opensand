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
 * @file Mpeg.h
 * @brief MPEG encapsulation plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#ifndef MPEG_CONTEXT_H
#define MPEG_CONTEXT_H

#include <EncapPlugin.h>
#include <NetPacket.h>
#include <NetBurst.h>
#include <opensand_conf/conf.h>

#include <vector>
#include <map>

#include <MpegEncapCtx.h>
#include <MpegDeencapCtx.h>

#define TS_PACKETSIZE 188   ///< The length of the MPEG2-TS packet (in bytes)
#define TS_HEADERSIZE 4     ///< The length of the MPEG2-TS header (in bytes)

/**
 * @class Mpeg
 * @brief MPEG encapsulation plugin implementation
 */
class Mpeg: public EncapPlugin
{
  public:

	/**
	 * @class Context
	 * @brief MPEG encapsulation / desencapsulation context
	 */
	class Context: public EncapContext
	{
	  private:

		/// Encapsulation contexts. Contexts are identified by an unique
		/// identifier (= PID)
		std::map < int, MpegEncapCtx * > encap_contexts;

		/// Deencapsulation contexts. Contexts are identified by an unique
		/// identifier (= PID)
		std::map < int, MpegDeencapCtx * > desencap_contexts;

		/// The packing threshold for encapsulation. Packing Threshold is the time
		/// the context can wait for additional SNDU packets to fill the incomplete
		/// MPEG packet before sending the MPEG packet with padding.
		unsigned long packing_threshold;

		/**
		 * Find the encapsulation context identified by the given PID
		 *
		 * @param pid       The PID to search for
		 * @param dest_spot The destination spot ID
		 * @return          The encapsulation context if successful,
		 *                  NULL otherwise
		 */
		MpegEncapCtx *find_encap_context(uint16_t pid, uint16_t spot_id);

	  public:

		/// constructor
		Context(EncapPlugin &plugin);

		/**
		 * Destroy the MPEG encapsulation / deencapsulation context
		 */
		~Context();

		void init();
		NetBurst *encapsulate(NetBurst *burst,
		                      map<long, int> &time_contexts);
		NetBurst *deencapsulate(NetBurst *burst);
		NetBurst *flush(int context_id);
		NetBurst *flushAll();

	  private:
		bool encapMpeg(NetPacket *packet, NetBurst *mpeg_packets,
		               int &context_id, long &time);
		bool deencapMpeg(NetPacket *packet, NetBurst *net_packets);

	};

	/**
	 * @class Packet
	 * @brief MPEG packet
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
		size_t getFixedLength() const {return TS_PACKETSIZE;};
		size_t getLength(const unsigned char *UNUSED(data)) const
		{
			return this->getFixedLength();
		};
		size_t getMinLength() const
		{
			return this->getFixedLength();
		};
		bool getChunk(NetPacket *packet, size_t remaining_length,
		              NetPacket **data, NetPacket **remaining_data) const;
		bool getSrc(const Data &data, tal_id_t &tal_id) const;
		bool getQos(const Data &data, qos_t &qos) const;
		
	};

	/// Constructor
	Mpeg();
};


CREATE(Mpeg, Mpeg::Context, Mpeg::PacketHandler, "MPEG2-TS");

#endif

