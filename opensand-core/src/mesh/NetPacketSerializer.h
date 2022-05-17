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
 * @file NetPacketSerializer.h
 * @brief This files defines a struct that serializes a NetPacket
 * @author Yohan Simard <yohan.simard@viveris.fr>
 */

#ifndef NET_PACKET_SERIALIZER_H
#define NET_PACKET_SERIALIZER_H

#include "NetPacket.h"
#include <array>
#include <cstdint>
#include <memory>

#define NET_PACKET_MAX_NAME_SIZE 32
#define NET_PACKET_MAX_DATA_SIZE 8000

struct __attribute__((__packed__)) net_packet_buffer_t
{
	spot_id_t spot;
	uint8_t qos;
	uint8_t src_tal_id;
	uint8_t dst_tal_id;
	uint16_t type;
	std::array<char, NET_PACKET_MAX_NAME_SIZE> name;
	uint32_t name_length;
	std::size_t header_length;
	uint32_t length;
	std::array<uint8_t, NET_PACKET_MAX_DATA_SIZE> data;

	net_packet_buffer_t(const NetPacket &pkt);
	std::unique_ptr<NetPacket> deserialize() const;
};

#endif