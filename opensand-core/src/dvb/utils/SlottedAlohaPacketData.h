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
 * @file SlottedAlohaPacketData.h
 * @brief The Slotted Aloha data packets
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
 */

#ifndef SALOHA_PACKET_DATA_H
#define SALOHA_PACKET_DATA_H

#include "SlottedAlohaPacket.h"
#include "OpenSandCore.h"

#include <stdlib.h>

typedef uint32_t saloha_pdu_id_t;

/// Slotted Aloha data packet header
typedef struct
{
	saloha_pdu_id_t id;             ///< ID of the PDU to which the packet belongs
	uint16_t ts;                    ///< Timeslot
	uint16_t seq;                   ///< Sequence of the packet in the PDU
	uint16_t pdu_nb;                ///< Number of packets in the PDU
	uint16_t nb_replicas;           ///< The number of replicas of this packet
	                                ///  per Slotted Aloha frame
	qos_t qos; // Duplicate header to transmit information TODO check if necessary
	uint16_t total_length;          ///< The packet total length
	uint16_t replicas[0];           ///< The TS for replicas
} __attribute__((__packed__)) saloha_data_hdr_t;


/**
 * @class SlottedAlohaPacketData
 * @brief Slotted Aloha data packets
 */
class SlottedAlohaPacketData: public SlottedAlohaPacket
{

 private:

	/// The packet timeout (in Slotted Aloha frames number)
	time_sf_t timeout_saf;

	/// The number of retransmissions of this packet
	uint16_t nb_retransmissions;
 public:

	/**
	 * Build a slotted Aloha data packet
	 *
	 * @param packet              packet to create
	 * @param id                  identidiant of initial packet
	 * @param ts                  time slot to send packet
	 * @param seq                 offset about initial packet
	 * @param pdu_nb              number of packets
	 * @param nb_replicas         number of replicas
	 * @param timeout             timeout before deleting
	 */
	SlottedAlohaPacketData(const Data &data,
	                       saloha_pdu_id_t id,
	                       uint16_t ts,
	                       uint16_t seq,
	                       uint16_t pdu_nb,
	                       uint16_t nb_replicas,
	                       time_sf_t timeout);

	SlottedAlohaPacketData(const Data &data, size_t length);

	/**
	 * Class destructor
	 */
	~SlottedAlohaPacketData();

	/**
	 * Get identifiant of initial packet
	 *
	 * @return identifiant of initial packet
	 */
	saloha_pdu_id_t getId() const;

	/**
	 * Get time slot to send packet
	 *
	 * @return time slot to send packet
	 */
	uint16_t getTs() const;

	/**
	 * Get offset
	 *
	 * @return offset
	 */
	uint16_t getSeq() const;

	/**
	 * Get number of packets
	 *
	 * @return number of packets
	 */
	uint16_t getPduNb() const;

	/**
	 * Get timeout before deleting
	 *
	 * @return timeout before deleting
	 */
	uint16_t getTimeout() const;

	/**
	 * Get number of retransmissions
	 *
	 * @return number of retransmissions
	 */
	uint16_t getNbRetransmissions() const;

	/**
	 * Get number of replicas
	 *
	 * @return number of replicas
	 */
	uint16_t getNbReplicas() const;

	/**
	 * Get the length of replicas
	 *
	 * @return the length of replicas
	 */
	size_t getReplicasLength() const;

	/**
	 * Get the nth replicas
	 *
	 * @param pos  the pos of the replicas
	 * @return the replicas at pos
	 */
	uint16_t getReplica(uint16_t pos) const;

	/**
	 * Get qos initial packet
	 *
	 * @return qos of initial packet
	 */
	uint8_t getQos() const;

	/**
	 * Set the time slot
	 */
	void setTs(uint16_t ts);

	/**
	 * Set the time slots or replicas
	 */
	void setReplicas(uint16_t* replicas,size_t nb_replicas);

	/**
	 * Return true if timeout is triggered, false otherwise
	 *
	 * @return true if timeout is triggered, false otherwise
	 */
	bool isTimeout() const;

	/**
	 * Set the timeout
	 *
	 * @param timeout_saf  The timeout value
	 */
	void setTimeout(time_sf_t timeout_saf);

	/**
	 * Decrease the timeout
	 */
	void decTimeout();

	/**
	 * Check if a packet can be retransmitted
	 *
	 * @param max_retransmissions  The maximum number of allowed retransmissions
	 *
	 * @return true if packet can be retranssmitted, false otherwise
	 */
	bool canBeRetransmitted(uint16_t max_retransmissions) const;

	/**
	 * Increase the number of retransmissions
	 */
	void incNbRetransmissions();


	/// implementation of virtual fonctions
	size_t getTotalLength() const;
	size_t getPayloadLength() const;
	Data getPayload() const;
	void setQos(uint8_t qos);
	saloha_id_t getUniqueId() const;

	/**
	 * Get the packet length from data
	 *
	 * @param data  The packet content
	 * @return the packet length
	 */
	static size_t getPacketLength(const Data &data);
};

/// A list of Slotted Aloha Data Packets
typedef vector<SlottedAlohaPacketData *> saloha_packets_data_t;

#endif

