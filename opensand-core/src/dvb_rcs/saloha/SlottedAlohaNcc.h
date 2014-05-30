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
 * @file SlottedAlohaNcc.h
 * @brief The Slotted Aloha
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 */

#ifndef SALOHA_NCC_H
#define SALOHA_NCC_H

#include "SlottedAloha.h"

#include "DvbFrame.h"
#include "NetBurst.h"
#include "EncapPlugin.h"
#include "TerminalContextSaloha.h"
#include "TerminalCategorySaloha.h"

#include <list>

/**
 * @class SlottedAlohaNcc
 * @brief The Slotted Aloha class for NCC
 */

class SlottedAlohaNcc: public SlottedAloha
{
 private:

	// Helper to simplify context manipulation
	typedef map<tal_id_t, TerminalContextSaloha *> saloha_terminals_t;

	/** List of registered terminals */
	saloha_terminals_t terminals;

	/// Method used to schedule Slotted Aloha
	// TODO RENAME into SlottedAlohaScheduling
	SlottedAlohaMethod *method;

	/// Traffic to simulate
	uint8_t simulation_traffic;

	typedef map<string, Probe<int> *> probe_per_cat_t;
	/// Statistics
	probe_per_cat_t probe_collisions;

 public:
	SlottedAlohaNcc();

	~SlottedAlohaNcc();

	/**
	 * Init the Slotted Aloha NCC class
	 *
	 * @return true on success, false otherwise
	 */

	bool init(void);

	/**
	 * Schedule Slotted Aloha packets
	 *
	 * @param burst                 burst to build containing packets to
	 *                              propagate to encap block
	 * @param complete_dvb_frames   frames to attach Slotted Aloha frame to send
	 * @param superframe_counter    current superframe counter
	 *
	 * @return true if packets was successful scheduled, false otherwise
	 */
	bool schedule(NetBurst **burst,
	              list<DvbFrame *> &complete_dvb_frames,
	              time_sf_t superframe_counter);

	// Implementation of a virtual functions
	bool onRcvFrame(DvbFrame *frame);

	/**
	 * @brief Add a new Slotted Aloha terminal context
	 *
	 * @return true on success, false otherwise
	 */
	bool addTerminal(tal_id_t tal_id);

 private:

	typedef enum
	{
		dup,
		no_prop,
		prop,
	} prop_state_t;

	/**
	 * Remove Slotted Aloha header
	 *
	 * @param packet The slotted aloha packet
	 * @return Encap packet without Slotted Aloha encapsulation
	 */
	NetPacket *removeHeader(SlottedAlohaPacketData *packet);

	/**
	 * Return if packet can be propagated to encap block or not
	 *
	 * @param last_propgated_id  The last ID propagated
	 * @param packet_id          The Slotted Aloha packet ID
	 * @param packet             The Slotted Aloha packet
	 *
	 * @return dup      if packet is duplicated or an error occurred
	 *                  (no propagation, no ACK),
	 *         no_prop  if packet cannot be propagated (no propagation but ACK),
	 *         prop     if packet can be propagated (propagation and ACK)
	 */
	prop_state_t canPropagate(saloha_id_t last_propgated_id,
	                          saloha_id_t packet_id,
	                          SlottedAlohaPacketData *packet);

	/**
	 * @brief Call a specific algorithm to remove all collided packets
	 *
	 * @param category  The terminal category
	 */
	void removeCollisions(TerminalCategorySaloha *category);

	/**
	 * @brief Simulate traffic to get some performance statistics with minimal plateform
	 *
	 * @param category  The terminal category
	 */
	void simulateTraffic(TerminalCategorySaloha *category);

	/**
	 * Schedule Slotted Aloha packets per category
	 *
	 * @param category              The category to schedule on
	 * @param burst                 burst to build containing packets to
	 *                              propagate to encap block
	 * @param complete_dvb_frames   frames to attach Slotted Aloha frame to send
	 *
	 * @return true if packets were successful scheduled, false otherwise
	 */
	bool scheduleCategory(TerminalCategorySaloha *category,
	                      NetBurst **burst,
	                      list<DvbFrame *> &complete_dvb_frames);
	//Implementation of virtual debug functions
	// TODO REMOVE
//	void debugFifo(const char *title);
};

/**
 * @class AlohaPacketComparator
 * @brief Functor to compare data Aloha packets for the std::sort function
 */
// TODO there is no id for packets !!!!!?????
class AlohaPacketComparator
{
 public:
	AlohaPacketComparator(uint16_t slots_per_carrier):
		slots_per_carrier(slots_per_carrier)
	{};

	/**
	 * Sort packets after removing algorithm to propagate packets to encap block
	 * in the correct order. This method is a std::sort callback called for
	 * each iteration
	 *
	 * @param pkt1  first packet to compare
	 * @param pkt2  second packet to compare
	 *
	 * @return true if order is good, false otherwise
	 */
	bool operator()(SlottedAlohaPacket *pkt1,
	                SlottedAlohaPacket *pkt2)
	{
		SlottedAlohaPacketData *data_pkt1 =
			dynamic_cast<SlottedAlohaPacketData *>(pkt1);
		SlottedAlohaPacketData *data_pkt2 =
			dynamic_cast<SlottedAlohaPacketData *>(pkt2);
		
		uint16_t replica_1 = data_pkt1->getReplica(0);
		uint16_t replica_2 = data_pkt2->getReplica(0);

		// First replica slot allows ordering
		// TODO in terminal, we use slots sorted in the entire category,
		// not per carrier => no module here !
		return (((replica_1 /*% this->slots_per_carrier*/) <
		         (replica_2 /*% this->slots_per_carrier*/)) &&
		        (pkt1->getSrcTalId()));
		// TODO SRC TAL ID ???? (eventually check if they have the same src !..
    };

 private:
	/// The slots per carrier
	uint16_t slots_per_carrier; // TODO we work per category, maybe not useful
};



#endif

