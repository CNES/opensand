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
 * @file SlottedAlohaTal.h
 * @brief The Slotted Aloha
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
*/

#ifndef SALOHA_TAL_H
#define SALOHA_TAL_H

#include "SlottedAloha.h"

#include "SlottedAlohaBackoff.h"
#include "SlottedAlohaFrame.h"
#include "SlottedAlohaAlgo.h"
#include "TerminalCategorySaloha.h"
#include "DvbFifo.h"
#include "DvbFrame.h"
#include "NetBurst.h"

#include <list>

/**
 * @class SlottedAlohaTal
 * @brief The Slotted Aloha
*/

class SlottedAlohaTal: public SlottedAloha
{
 private:

	/// The terminal ID
	tal_id_t tal_id;

	/// packet timeout in Slotted Aloha frame number
	time_sf_t timeout_saf;

	/// The packets waiting for ACK
	map<qos_t, saloha_packets_data_t> packets_wait_ack;

	/// list of  packets to be retransmitted
	saloha_packets_data_t retransmission_packets;

	/// Number of successive transmissions
	uint16_t nb_success;

	/// Maximum number of packets per superframe
	uint16_t nb_max_packets;

	/// Number of replicas per packet
	uint16_t nb_replicas;

	/// Configuration parameter : maximum number of retransmissions before
	/// packet deleting
	uint16_t nb_max_retransmissions;

	/// Current packet identifiant
	saloha_pdu_id_t base_id;

	/// Backoff algorithm used
	SlottedAlohaBackoff *backoff;

	/// The terminal category
	TerminalCategorySaloha *category;

	/// The DVB fifos
	fifos_t dvb_fifos;

	//TODO in opensandcore.h
	typedef map<qos_t, Probe<int> *> probe_per_qos_t;
	/// Statistics
	probe_per_qos_t probe_retransmission;
	probe_per_qos_t probe_wait_ack;
	probe_per_qos_t probe_drop;
	Probe<int> *probe_backoff;

 public:

	/**
	 * Class constructor whithout any parameters
	 */
	SlottedAlohaTal();

	/**
	 * Class destructor
	 */
	~SlottedAlohaTal();

	/**
	 * @brief Initialize Slotted Aloha for terminal
	 *
	 * @param tal_id                  The terminal ID
	 * @param category                The terminal category
	 * @param dvb_fifos               The DVB fifos
	 *
	 * @return true on success, false otherwise
	 */
	bool init(tal_id_t tal_id,
	          TerminalCategorySaloha *category,
	          const fifos_t &dvb_fifos);

	/**
	 * Add the Slotted Aloha header on an encapsulation packet
	 *
	 * @param encap_packet  encap packet received
	 * @param offset        offset of packet about initial packet
	 * @param burst_size    number of packets
	 *
	 * @return Slotted Aloha packet
	 */
	SlottedAlohaPacketData* addSalohaHeader(NetPacket *encap_packet,
	                                        uint16_t offset,
	                                        uint16_t burst_size);

	/**
	 * Schedule Slotted Aloha packets
	 *
	 * @param complete_dvb_frames  frames to attach Slotted Aloha frame to send
	 * @param sf_counter           current superframe counter
	 *
	 * @return true if packets was successful scheduled, false otherwise
	 */
	bool schedule(list<DvbFrame *> &complete_dvb_frames,
	              time_sf_t sf_counter);

	//Implementation of a virtual function
	bool onRcvFrame(DvbFrame *frame);

 private:

	/**
	 * generate random unique time slots for packets to send
	 *
	 * @return set containing random unique time slots
	 */
	saloha_ts_list_t getTimeSlots(void);

	/**
	 * Add a Slotted Aloha data packet and its replicas into Slotted Aloha frames
	 *
	 * @param complete_dvb_frames  frames to attach Slotted Aloha frame to send
	 * @param frame                current Slotted Aloha frame to fill
	 * @param sa_packet            Slotted Aloha packet to add into the frame
	 * @param slot                 slots to set for replicas
	 * @param qos                  qos of the packet
	 * @return true if the packet was successful added, false otherwise
	 */
	bool addPacketInFrames(list<DvbFrame *> &complete_dvb_frames,
	                       SlottedAlohaFrame **frame,
	                       SlottedAlohaPacketData *packet,
	                       saloha_ts_list_t::iterator &slot,
	                       qos_t qos);
};

#endif

