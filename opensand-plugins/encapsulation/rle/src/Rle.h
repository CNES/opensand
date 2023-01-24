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
 * @file Rle.h
 * @brief RLE encapsulation plugin implementation
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef RLE_CONTEXT_H
#define RLE_CONTEXT_H


#include <map>
#include <string>
#include <vector>

#include <EncapPlugin.h>
#include "RleIdentifier.h"


extern "C"
{
	#include <rle.h>
}


class NetPacket;


typedef enum {
	rle_alpdu_crc,
	rle_alpdu_sequence_number
} rle_alpdu_protection_t;


/**
 * @class Rle
 * @brief RLE encapsulation plugin implementation
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

		void loadRleConf(const struct rle_config &conf);
		bool init();

		Rt::Ptr<NetBurst> encapsulate(Rt::Ptr<NetBurst> burst, std::map<long, int> &time_contexts);
		Rt::Ptr<NetBurst> deencapsulate(Rt::Ptr<NetBurst> burst);
		Rt::Ptr<NetBurst> flush(int context_id);
		Rt::Ptr<NetBurst> flushAll();

	  private:
		/// RLE configuration
		struct rle_config rle_conf;

		/// Receivers identified by an unique identifier
		std::map<RleIdentifier *, struct rle_receiver *, ltRleIdentifier> receivers;

		bool decapNextPacket(Rt::Ptr<NetPacket> packet, NetBurst &burst);
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

		// Transmitters and partial sent packets list
		typedef std::pair<struct rle_transmitter *, std::vector<NetPacket *> > rle_trans_ctxt_t;
		std::map<RleIdentifier *, rle_trans_ctxt_t, ltRleIdentifier> transmitters;

	  public:
		PacketHandler(EncapPlugin &plugin);
		~PacketHandler();

		void loadRleConf(const struct rle_config &conf);
		bool init();

		Rt::Ptr<NetPacket> build(const Rt::Data &data,
		                         size_t data_length,
		                         uint8_t qos,
		                         uint8_t src_tal_id,
		                         uint8_t dst_tal_id) const override;
		size_t getFixedLength() const {return 0;};
		size_t getMinLength() const {return 3;};
		size_t getLength(const unsigned char *data) const;
		bool getSrc(const Rt::Data &data, tal_id_t &tal_id) const;
		bool getQos(const Rt::Data &data, qos_t &qos) const;

		bool encapNextPacket(Rt::Ptr<NetPacket> packet,
		                     std::size_t remaining_length,
		                     bool new_burst,
		                     Rt::Ptr<NetPacket> &encap_packet,
		                     Rt::Ptr<NetPacket> &remaining_data) override;

		bool getEncapsulatedPackets(Rt::Ptr<NetContainer> packet,
		                            bool &partial_decap,
		                            std::vector<Rt::Ptr<NetPacket>> &decap_packets,
		                            unsigned int decap_packet_count = 0) override;

		bool checkPacketForHeaderExtensions(Rt::Ptr<NetPacket> &packet) override;

		bool setHeaderExtensions(Rt::Ptr<NetPacket> packet,
		                         Rt::Ptr<NetPacket>& new_packet,
		                         tal_id_t tal_id_src,
		                         tal_id_t tal_id_dst,
		                         std::string callback_name,
		                         void *opaque) override;

		bool getHeaderExtensions(const Rt::Ptr<NetPacket>& packet,
		                         std::string callback_name,
		                         void *opaque) override;

	 protected:
		bool getChunk(Rt::Ptr<NetPacket> packet,
		              std::size_t remaining_length,
		              Rt::Ptr<NetPacket> &data,
		              Rt::Ptr<NetPacket> &remaining_data) const override;
	};

	/// Constructor
	Rle();
	~Rle();

	/**
	 * @brief Generate the configuration for the plugin
	 */
	static void generateConfiguration(const std::string &parent_path,
	                                  const std::string &param_id,
	                                  const std::string &plugin_name);

	bool init();

	static bool getLabel(const NetPacket &packet, uint8_t label[]);
	static bool getLabel(const Rt::Data &data, uint8_t label[]);
};


CREATE(Rle, Rle::Context, Rle::PacketHandler, "RLE");


#endif
