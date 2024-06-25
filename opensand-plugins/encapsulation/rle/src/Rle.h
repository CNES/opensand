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
 * @brief Rle encapsulation plugin implementation
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef Rle_CONTEXT_H
#define Rle_CONTEXT_H

#include <map>
#include <string>
#include <vector>

#include <SimpleEncapPlugin.h>
#include "RleIdentifier.h"

extern "C"
{
#include <rle.h>
}

class NetPacket;

typedef enum
{
	rle_alpdu_crc,
	rle_alpdu_sequence_number
} rle_alpdu_protection_t;

/**
 * @class Rle
 * @brief Rle encapsulation plugin implementation
 */
class Rle : public SimpleEncapPlugin
{
public:
	Rle();
	~Rle();

private:
	
	// protected:
	// bool decapNextPacket(Rt::Ptr<NetPacket> packet, NetBurst &burst) override;
public:
	/// rle configuration
	struct rle_config rle_conf;
	void loadRleConf(const struct rle_config &conf);

	/// Receivers identified by an unique identifier
	std::map<RleIdentifier *, struct rle_receiver *, ltRleIdentifier> receivers;
	bool decapNextPacket(Rt::Ptr<NetPacket> packet, std::vector<Rt::Ptr<NetPacket>> &decap_packets);
	Rt::Ptr<NetPacket> build(const Rt::Data &data,
							 size_t data_length,
							 uint8_t qos,
							 uint8_t src_tal_id,
							 uint8_t dst_tal_id) override;

	bool getSrc(const Rt::Data &data, tal_id_t &tal_id) const;
	bool getDst(const Rt::Data &data, tal_id_t &tal_id) const;
	bool getQos(const Rt::Data &data, qos_t &qos) const;
	bool encapNextPacket(Rt::Ptr<NetPacket> packet,
						 std::size_t remaining_length,
						 bool new_burst,
						 Rt::Ptr<NetPacket> &encap_packet,
						 Rt::Ptr<NetPacket> &remaining_data) override;
	bool decapAllPackets(Rt::Ptr<NetContainer> encap_packets,
						 std::vector<Rt::Ptr<NetPacket>> &decap_packets,
						 unsigned int decap_packet_count = 0) override;

	bool setHeaderExtensions(Rt::Ptr<NetPacket> packet,
							 Rt::Ptr<NetPacket> &new_packet,
							 tal_id_t tal_id_src,
							 tal_id_t tal_id_dst,
							 std::string callback_name,
							 void *opaque) override;

	bool getHeaderExtensions(const Rt::Ptr<NetPacket> &packet,
							 std::string callback_name,
							 void *opaque) override;
	static bool getLabel(const NetPacket &packet, uint8_t label[]);
	static bool getLabel(const Rt::Data &data, uint8_t label[]);

	// Transmitters and partial sent packets list
	typedef std::pair<struct rle_transmitter *, std::vector<NetPacket *>> rle_trans_ctxt_t;
	std::map<RleIdentifier *, rle_trans_ctxt_t, ltRleIdentifier> transmitters;

	/**
	 * @brief Generate the configuration for the plugin
	 */
	static void generateConfiguration(const std::string &parent_path,
									  const std::string &param_id,
									  const std::string &plugin_name);

	bool init() override;
};

CREATE(Rle, "RLE");

#endif
