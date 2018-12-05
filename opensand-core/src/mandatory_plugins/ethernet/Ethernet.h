/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
 * Copyright © 2018 CNES
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
 * @file Ethernet.h
 * @brief Ethernet LAN adaptation plugin implementation
 * @author Remy PIENNE <rpienne@toulouse.viveris.com>
 * This LAN adaptation plugin can be used in two configurations:
 * - The host uses a TAP interface, Ethernet is first on the stack, this plugin
 * reads and writes Ethernet frames.
 * - The host uses a TUN interface, IP is first on the stack, this plugin reads
 * IP packets and encapsulates them in Ethernet frames for lower layers, and
 * does the same in reverse for writing to the IP layer.
 * In both configurations, it handles NetPacket metadata.
 * Different Ethernet protocols are handled: Basic Ethernet II, 802.1Q and 802.1ad
 * The protocol is set when calling the constructor, by reading the
 * configuration file. It defaults to Ethernet II.
 */

#ifndef ETH_CONTEXT_H
#define ETH_CONTEXT_H

#include "EthernetHeader.h"
#include "Evc.h"

#include <NetBurst.h>
#include <MacAddress.h>
#include <NetPacket.h>
#include <LanAdaptationPlugin.h>
#include <TrafficCategory.h>
#include <opensand_output/Output.h>
#include <opensand_conf/ConfigurationFile.h>

#include <cassert>
#include <vector>
#include <map>

/**
 * @class Ethernet
 * @brief ETH lan adaptation plugin implementation
 */
class Ethernet: public LanAdaptationPlugin
{
  public:
	Ethernet();

	bool init();

	/**
	 * @class Context
	 * @brief ETH context
	 */
	class Context: public LanAdaptationContext
	{
	  public:

		/// constructor
		Context(LanAdaptationPlugin &plugin);

		/**
		 * Destroy the  context
		 */
		~Context();

		bool init();
		NetBurst *encapsulate(NetBurst *burst, map<long, int> &(time_contexts));
		NetBurst *deencapsulate(NetBurst *burst);
		char getLanHeader(unsigned int pos, NetPacket *packet);
		bool handleTap();
		void updateStats(unsigned int period);
		bool initLanAdaptationContext(
			tal_id_t tal_id,
			tal_id_t gw_id,
			sat_type_t satellite_type,
			const SarpTable *sarp_table);

	  protected:

		/**
		 * @brief create an Ethernet frame from IP data
		 *
		 * @param packet     The upper or network packet
		 * @param evc_id     The id of the EVC if found
		 * @return the Ethernet frame
		 */
		NetPacket *createEthFrameData(NetPacket *packet, uint8_t &evc_id);

		/**
		 * @brief create an Ethernet frame from IP data
		 *        and other information
		 *
		 * @param data               The upper or network packet data
		 * @param mac_src            The source MAC address
		 * @param mac_dst            The destination MAC address
		 * @param ether_type         The payload EtherType
		 * @param q_tci              The Q TCI
		 * @param ad_tci             The AD TCI
		 * @param qos                The packet QoS
		 * @param src_tal_id         The source terminal ID
		 * @param dst_tal_id         The destination terminal ID
		 * @param desired_frame_type The frame type we want to build
		 * @return the Ethernet frame
		 */
		NetPacket *createEthFrameData(Data data,
		                              MacAddress mac_src, MacAddress mac_dst,
		                              uint16_t ether_type,
		                              uint16_t q_tci, uint16_t ad_tci,
		                              qos_t qos,
		                              tal_id_t src_tal_id, tal_id_t dst_tal_id,
		                              uint16_t desired_frame_type);

		/**
		 * @brief Get the EVC corresponding to Ethernet flow
		 *
		 * @param src_mac    The source MAC address
		 * @param dst_mac    The destination MAC address
		 * @param ether_type The EtherType
		 * @param evc_id     The id of the EVC if found
		 * @return the EVC if found, NULL otherwise
		 */
		Evc *getEvc(const MacAddress src_mac,
		            const MacAddress dst_mac,
		            uint16_t ether_type,
		            uint8_t &evc_id) const;

		/**
		 * @brief Get the EVC corresponding to Ethernet flow
		 *
		 * @param src_mac    The source MAC address
		 * @param dst_mac    The destination MAC address
		 * @param q_tci      The Q TCI
		 * @param ether_type The EtherType
		 * @param evc_id     The id of the EVC if found
		 * @return the EVC if found, NULL otherwise
		 */
		Evc *getEvc(const MacAddress src_mac,
		            const MacAddress dst_mac,
		            uint16_t q_tci,
		            uint16_t ether_type,
		            uint8_t &evc_id) const;

		/**
		 * @brief Get the EVC corresponding to Ethernet flow
		 *
		 * @param src_mac    The source MAC address
		 * @param dst_mac    The destination MAC address
		 * @param q_tci      The Q TCI
		 * @param ad_tci     The ad TCI
		 * @param ether_type The EtherType
		 * @param evc_id     The id of the EVC if found
		 * @return the EVC if found, NULL otherwise
		 */
		Evc *getEvc(const MacAddress src_mac,
		            const MacAddress dst_mac,
		            uint16_t q_tci,
		            uint16_t ad_tci,
		            uint16_t ether_type,
		            uint8_t &evc_id) const;

		/**
		 * @brief Initialize the statistics
		 */
		void initStats();

		/**
		 * @brief Initialize the EVC from configuration
		 *
		 * @param config  The configuration elements
		 * @return true on success, false otherwise
		 */
		bool initEvc(ConfigurationFile &config);

		/**
		 * @brief Initialize the traffic categories from IP configuration
		 *
		 * @todo remove this
		 *
		 * @param config  The configuration elements
		 * @return true on success, false otherwise
		 */
		bool initTrafficCategories(ConfigurationFile &config);

		/// The configuration
		ConfigurationFile config;

		/// The Ethernet Virtual Connections
		map<uint8_t, Evc *> evc_map;
		/// The amount of data sent per EVC between two updates
		map<uint8_t, size_t> evc_data_size;
		/// The throughput per EVC
		map<uint8_t, Probe<float> *> probe_evc_throughput;
		/// The frame size per EVC
		map<uint8_t, Probe<float> *> probe_evc_size;

		uint16_t lan_frame_type; //< The type of Ethernet frame forwarded on LAN
		uint16_t sat_frame_type; //< The type of Ethernet frame transmitted on satellite

		/// The traffic categories
		map<qos_t, TrafficCategory *> category_map;

		/// The default traffic category
		qos_t default_category;

	};

	/**
	 * @class PacketHandler
	 * @brief ETH frame handler
	 */
	class PacketHandler: public LanAdaptationPacketHandler
	{

	  public:

		PacketHandler(LanAdaptationPlugin &plugin):
			LanAdaptationPlugin::LanAdaptationPacketHandler(plugin)
		{};

		size_t getFixedLength() const {return 0;};

		size_t getLength(const unsigned char *data) const
		{
			size_t length = 0;
			if(data == NULL)
			{
				//error
				return 0;
			}
			return length;
		}

		string getName() const {return "Ethernet";}

		NetPacket *build(const Data &data,
		                 size_t data_length,
		                 uint8_t qos,
		                 uint8_t src_tal_id,
		                 uint8_t dst_tal_id) const;

	};

  private:

	/**
	 * @brief Retrieve the type of frame
	 *
	 * @param data   the Ethernet frame data
	 * @return the type of frame
	 */
	static uint16_t getFrameType(const Data &data);

	/**
	 * @brief Retrieve the EtherType of a payload carried by an Ethernet frame
	 *
	 * @param data   the Ethernet frame data
	 * @return the EtherType
	 */
	static uint16_t getPayloadEtherType(const Data &data);

	/**
	 * @brief Retrieve the Q TCI from an Ethernet frame
	 *
	 * @param data   the Ethernet frame data
	 * @return the Q TCI
	 */
	static uint16_t getQTci(const Data &data);

	/**
	 * @brief Retrieve the ad TCI from an Ethernet frame
	 *
	 * @param data   the Ethernet frame data
	 * @return the ad TCI
	 */
	static uint16_t getAdTci(const Data &data);

	/**
	 * @brief Retrieve the source MAC address from an Ethernet frame
	 *
	 * @param data   the Ethernet frame data
	 * @return the source MAC address on success, an empty sring otherwise
	 */
	static MacAddress getSrcMac(const Data &data);

	/**
	 * @brief Retrieve the destination MAC address from an Ethernet frame
	 *
	 * @param data   the Ethernet frame data
	 * @return the destination MAC address on success, an empty sring otherwise
	 */
	static MacAddress getDstMac(const Data &data);

};

CREATE(Ethernet, Ethernet::Context, Ethernet::PacketHandler, "Ethernet");


#endif

