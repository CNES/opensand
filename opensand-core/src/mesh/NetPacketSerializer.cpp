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
 * @file NetPacketSerializer.cpp
 * @brief This files defines a struct that serializes a NetPacket
 * @author Yohan Simard <yohan.simard@viveris.fr>
 */

#include "NetPacketSerializer.h"
#include "NetPacket.h"

net_packet_buffer_t::net_packet_buffer_t(const NetPacket &pkt):
    spot{pkt.getSpot()},
    qos{pkt.getQos()},
    src_tal_id{pkt.getSrcTalId()},
    dst_tal_id{pkt.getDstTalId()},
    type{pkt.getType()},
    header_length{pkt.getHeaderLength()}
{
	auto pkt_name = pkt.getName();
	name_length = pkt_name.size();
	std::copy(pkt_name.begin(), pkt_name.end(), name.begin()); // Try std::move?
	auto pkt_data = pkt.getData();
	length = pkt_data.size();
	std::copy(pkt_data.begin(), pkt_data.end(), data.begin()); // Try std::move?
}

std::unique_ptr<NetPacket> net_packet_buffer_t::deserialize() const
{
	std::string name_str{name.data(), name_length};
	Data data_str{data.data(), length};
	auto pkt = std::unique_ptr<NetPacket>{
	    new NetPacket{data_str,
	                  length,
	                  name_str,
	                  type,
	                  qos,
	                  src_tal_id,
	                  dst_tal_id,
	                  header_length}};
    pkt->setSpot(spot);
	return pkt;
}
