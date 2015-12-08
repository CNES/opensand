/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 * Copyright © 2015 CNES
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
 * @file SlottedAlohaPacketCtrl.h
 * @brief The Slotted Aloha control signal packets
 *
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
 */

#ifndef SALOHA_PACKET_CTRL_H
#define SALOHA_PACKET_CTRL_H

#include "SlottedAlohaPacket.h"

/// Slotted Aloha Control packet header
typedef struct
{
	uint8_t type;           ///< The type of Slotted Aloha Control
	uint16_t total_length;  ///< The length of the packet
	tal_id_t tal_id;        ///< The destination terminal
} __attribute__((__packed__)) saloha_ctrl_hdr_t;


/**
 * @class SlottedAlohaPacketCtrl
 * @brief Slotted Aloha control signal packets
 */
class SlottedAlohaPacketCtrl: public SlottedAlohaPacket
{
public:
	/**
	 * Build a slotted Aloha control packet
	 *
	 * @param data       data to store in packet
	 * @param length     data length
	 * @param ctrl_type  control type of packet to create
	 * @param tal_id     the destination terminal ID
	 */
	SlottedAlohaPacketCtrl(const Data &data,
	                       uint8_t ctrl_type,
	                       tal_id_t tal_id);

	/**
	 * Build a slotted Aloha control packet
	 *
	 * @param data    data to convert to packet
	 * @param length  data length
	 */
	SlottedAlohaPacketCtrl(const unsigned char* data, size_t length);

	/**
	 * Class destructor
	 */
	~SlottedAlohaPacketCtrl();

	/**
	 * Get the ID carried by the control packet
	 *
	 * @return the ID carried by the packet
	 */
	saloha_id_t getId() const;

	/**
	 * Get the control type of packet
	 *
	 * @return control type of packet
	 */
	uint8_t getCtrlType() const;

	/**
	 * Get the destination terminal ID for the packet
	 *
	 * @return the terminal ID
	 */
	tal_id_t getTerminalId() const;

	/// implementation of virtual fonctions
	size_t getTotalLength() const;
	saloha_id_t getUniqueId() const;

	/**
	 * Get the packet length from data
	 *
	 * @param data  The packet content
	 * @return the packet length
	 */
	static size_t getPacketLength(const Data &data);
};

#endif

