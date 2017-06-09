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
 * @file Rohc.h
 * @brief ROHC LAN adaptation plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#ifndef ROHC_CONTEXT_H
#define ROHC_CONTEXT_H

#include <LanAdaptationPlugin.h>
#include <NetPacket.h>
#include <NetBurst.h>
#include <cassert>

#include <vector>
#include <map>


#include <RohcPacket.h>

#include <rohc/rohc.h>
#include <rohc/rohc_comp.h>
#include <rohc/rohc_decomp.h>

#define MAX_ROHC_SIZE      (5 * 1024)

/**
 * @class Rohc
 * @brief ROHC compression plugin implementation
 */
class Rohc: public LanAdaptationPlugin
{
  public:

	/**
	 * @class Context
	 * @brief ROHC compression / decompression context
	 */
	class Context: public LanAdaptationContext
	{
	  private:

		/// The ROHC compressor
		struct rohc_comp *comp;
		/// The ROHC decompressors
		std::map<uint8_t, struct rohc_decomp*> decompressors;

	  public:

		/// constructor
		Context(LanAdaptationPlugin &plugin);

		/**
		 * Destroy the ROHC compression / decompression context
		 */
		~Context();

		void init();
		NetBurst *encapsulate(NetBurst *burst,
		                      std::map<long, int> &(time_contexts));
		NetBurst *deencapsulate(NetBurst *burst);
		char getLanHeader(unsigned int UNUSED(pos),
		                  NetPacket *UNUSED(packet))
		{
			assert(0);
		};

	  private:

		bool compressRohc(NetPacket *packet, NetPacket **comp_packet);
		bool decompressRohc(NetPacket *packet, NetPacket **dec_packet);

		/**
		 * Removes the frame's Ethernet header and copies the
		 * contained packet in a new NetPacket.
		 * @warning head_buffer should be able to contain a
		 *          complete Ethernet Frame as it will be used for rebuild
		 *
		 * @param frame        The Ethernet frame
		 * @param head_length  The size of the Ethernet header
		 * @param head_buffer  The buffer containing the old Ethernet header
		 * @param payload      The packet contained in the Ethernet frame
		 * @return true on success, false otherwise
		 */
		bool extractPacketFromEth(NetPacket *frame,
		                          size_t &head_length,
		                          unsigned char *head_buffer,
		                          NetPacket **payload);

		/**
		 * Adds an Ethernet header to the packet and copies the frame
		 * in a new NetPacket.
		 * @warning head_buffer should be able to contain a
		 *          complete Ethernet Frame as it wuill be uszed for rebuild
		 *
		 * @param packet       The packet contained in the Ethernet frame
		 * @param head_length  The size of the Ethernet header
		 * @param head_buffer  The buffer containing the old Ethernet header
		 * @param eth_frame    The Ethernet frame
		 * @return true on success, false otherwise
		 */
		bool buildEthFromPacket(NetPacket *packet,
		                        size_t head_length,
		                        unsigned char *head_buffer,
		                        NetPacket **eth_frame);
		bool handleTap() {return false;};
	};

	/**
	 * @class Packet
	 * @brief ROHC packet
	 */
	class PacketHandler: public LanAdaptationPacketHandler
	{

	  public:

		PacketHandler(LanAdaptationPlugin &plugin);

		NetPacket *build(const Data &data,
		                 size_t data_length,
		                 uint8_t qos,
		                 uint8_t src_tal_id,
		                 uint8_t dst_tal_id) const;

		size_t getLength(const unsigned char *UNUSED(data)) const {return 0;};
		size_t getFixedLength() const {return 0;};

	};

	/// Constructor
	Rohc();
};

CREATE(Rohc, Rohc::Context, Rohc::PacketHandler, "ROHC");

#endif

