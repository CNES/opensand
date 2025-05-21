/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @file GseRust.h
 * @brief GseRust encapsulation plugin implementation
 * @author Axel Pinel <axel.pinel@viveris.com>
 */

#ifndef GseRust_CONTEXT_H
#define GseRust_CONTEXT_H

#include <map>
#include <string>
#include <vector>

#include <SimpleEncapPlugin.h>
#include <EncapPlugin.h>
#include "GseIdentifier.h"
#include "SimpleGseRust.h"
#include "Memory.h"
#include "GseRustCApi.h"

class NetPacket;
class NetBurst;

/**
 * @class GseRust
 * @brief GseRust encapsulation plugin implementation
 */
class GseRust : public EncapPlugin, SimpleGseRust
{
public:
	/**
	 * @class Context
	 * @brief GseRust encapsulation / desencapsulation context
	 */

	class Context : public EncapContext
	{
	public:
		Context(EncapPlugin &plugin);
		~Context();
		
		std::string getName() const
		{
			return StackPlugin::StackContext::getName();
		}
		/**
		 * @warning This method does nothing but must exist.
		 */
		bool init() override;


		// require for compatibility 
		void setFilterTalId(uint8_t tal_id) override;
		/**
		 * @warning This method does nothing but must exist.
		 */
		Rt::Ptr<NetBurst> encapsulate(Rt::Ptr<NetBurst> burst, std::map<long, int> &time_contexts) override;

		/**
		 * @brief this method drops packet if destination ID is not this current ID
		 * @warning This method does not deencapsulate
		 */
		Rt::Ptr<NetBurst> deencapsulate(Rt::Ptr<NetBurst> burst) override;

		/**
		 * @warning This method should never be called. If called, it will abort.
		 */
		Rt::Ptr<NetBurst> flush(int context_id) override;

		/**
		 * @warning This method should never be called. If called, it will abort.
		 */
		Rt::Ptr<NetBurst> flushAll() override;

		private:
		GseRust *plugin;
	};

	/**
	 * @class Packet
	 * @brief GseRust packet
	 */
	class PacketHandler : public EncapPacketHandler
	{
	private: 
			
		GseRust *plugin;


	public:
		PacketHandler(EncapPlugin &plugin);
		~PacketHandler();

		bool getEncapsulatedPackets(Rt::Ptr<NetContainer> packet,
									bool &partial_decap,
									std::vector<Rt::Ptr<NetPacket>> &decap_packets,
									unsigned int decap_packet_count = 0) override;

		/**
		 * @warning This method should never be called. If called, it will abort.
		 */
		Rt::Ptr<NetPacket> build(const Rt::Data &data,
								 std::size_t data_length,
								 uint8_t qos,
								 uint8_t src_tal_id,
								 uint8_t dst_tal_id) override;


		size_t getFixedLength() const override { return 0; };

		size_t getMinLength() const override { return 3; };

		/**
		 * @warning This method should never be called. If called, it will abort.
		 */
		size_t getLength(const unsigned char *data) const override;

		/**
		 * @warning This method should never be called. If called, it will abort.
		 */
		bool getSrc(const Rt::Data &data, tal_id_t &tal_id) const override;

		/**
		 * @warning This method should never be called. If called, it will abort.
		 */
		bool getDst(const Rt::Data &data, tal_id_t &tal_id) const override;

		/**
		 * @warning This method should never be called. If called, it will abort.
		 */
		bool getQos(const Rt::Data &data, qos_t &qos) const override;

		/**
		 * @warning This method should never be called. If called, it will abort.
		 */
		bool checkPacketForHeaderExtensions(Rt::Ptr<NetPacket> &packet) override;

		/**
		 * @warning This method should never be called. If called, it will abort.
		 */
		bool setHeaderExtensions(Rt::Ptr<NetPacket> packet,
								 Rt::Ptr<NetPacket> &new_packet,
								 tal_id_t tal_id_src,
								 tal_id_t tal_id_dst,
								 std::string callback,
								 void *opaque) override;

		/**
		 * @warning This method should never be called. If called, it will abort.
		 */
		bool getHeaderExtensions(const Rt::Ptr<NetPacket> &packet,
								 std::string callback,
								 void *opaque) override;

	protected:
		/**
		 * @brief   Encapsulate and return a packet of remaining_length bytes in
		 * @warning If encapsulation is partial (i.e. packet is fragmented), the method needs to return the entire packet in @c remaining_data
		 * @param   packet  IN: The NetPacket to encapsulate
		 * @param   remaining_length  IN: The remaining length in the BBFrame
		 * @param   data OUT: The packet encapsulated, it can contain the entire packet, a fragment, or padding bytes.
		 * @param   remaining_data OUT:  The remaining data if the packet does'nt entirely fit in the BBframe. If the packet is fragmented, must contain the @c entire packet
		 * @return  true on success, false otherwise.
		 */
		bool getChunk(Rt::Ptr<NetPacket> packet,
					  std::size_t remaining_length,
					  Rt::Ptr<NetPacket> &data,
					  Rt::Ptr<NetPacket> &remaining_data) override;
	};

	/// Constructor
	GseRust();
	~GseRust();
	bool init();

	/**
	 * @brief Generate the configuration for the plugin
	 */
	static void generateConfiguration(const std::string &parent_path,
									  const std::string &param_id,
									  const std::string &plugin_name) ;

	// Static methods: getter/setter for label/fragId

	/**
	 * @brief  Set the GseRust packet label
	 *
	 * @param   packet  The packet to get label values from.
	 * @param   label   The label to set values of.
	 * @return  true on success, false otherwise.
	 */
	static bool setLabel(const NetPacket &packet, uint8_t label[]);

	/**
	 * @brief   Get the source TAL Id from label.
	 * @param   label  The label to read value from.
	 * @return  the source TAL Id.
	 */
	static uint8_t getSrcTalIdFromLabel(const uint8_t label[]);

	/**
	 * @brief   Get the destination TAL Id from label.
	 * @param   label  The label to read value from.
	 * @return  the destination TAL Id.
	 */
	static uint8_t getDstTalIdFromLabel(const uint8_t label[]);

	/**
	 * @brief   Get the QoS value from label.
	 * @param   label  The label to read value from.
	 * @return  the QoS value.
	 */
	static uint8_t getQosFromLabel(const uint8_t label[]);

	/**
	 * @brief   Create a fragment id from a packet.
	 * @param   packet  The packet to create the frag id from..
	 * @return  the frag id.
	 */
	static uint8_t getFragId(const NetPacket &packet) ;

	/**
	 * @brief   Get the source TAL Id from a fragment id..
	 * @param   frag_id  The fragment dd to read value from.
	 * @return  the source TAL Id.
	 */
	static uint8_t getSrcTalIdFromFragId(const uint8_t frag_id) ;

	/**
	 * @brief   Get the QoS value from a fragment id..
	 * @param   frag_id  The fragment dd to read value from.
	 * @return  the QoS value.
	 */
	static uint8_t getQosFromFragId(const uint8_t frag_id) ;
	std::string getName() const
		{
			return StackPlugin::getName();
		}
};

CREATE(GseRust, GseRust::Context, GseRust::PacketHandler, "GSERust");

#endif
