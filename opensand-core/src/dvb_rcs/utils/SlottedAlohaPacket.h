/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
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
 * @file SlottedAlohaPacketData.h
 * @brief The Slotted Aloha packets
 * @author Julien Bernard / Viveris technologies
 */

#ifndef SALOHA_PACKET_H
#define SALOHA_PACKET_H

#include "NetPacket.h"
#include "OpenSandCore.h"

#include <stdlib.h>


//Control signal types
#define SALOHA_CTRL_ERR 0
#define SALOHA_CTRL_ACK 1

/// <ID,Seq,PDU_nb,QoS> constant identifiers
#define SALOHA_ID_ID 0
#define SALOHA_ID_SEQ 1
#define SALOHA_ID_PDU_NB 2
#define SALOHA_ID_QOS 3


/**
 * @class SlottedAlohaPacket
 * @brief Slotted Aloha packet parent
 */
class SlottedAlohaPacket: public NetPacket
{
public:

	SlottedAlohaPacket(const Data &data):
		NetPacket(data)
	{};

	SlottedAlohaPacket(const Data &data, size_t length):
		NetPacket(data, length)
	{};

	SlottedAlohaPacket(const unsigned char *data, size_t length):
		NetPacket(data, length)
	{};

	/**
	 * Class destructor
	 */
	virtual ~SlottedAlohaPacket() {};

};

/// A list of Slotted Aloha Packets 
typedef vector<SlottedAlohaPacket *> saloha_packets_t;
/// A Slotted Aloha ID representation
typedef Data saloha_id_t;


#endif

