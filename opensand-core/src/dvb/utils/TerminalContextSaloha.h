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
 * @file TerminalContextSaloha.h
 * @brief The termial context for Slotted Aloha
 * @author Julien Bernard / Viveris Technologies
 *
 */


#ifndef _TERMINAL_CONTEXT_SALOHA_H_
#define _TERMINAL_CONTEXT_SALOHA_H_

#include "TerminalContext.h"

#include "SlottedAlohaPacketData.h"

#include <map>

using std::map;

typedef enum
{
	no_prop,
	prop,
} prop_state_t;

/**
 * @class TerminalContextSaloha
 * @brief Interface for a terminal context to be used in Slotted Aloha.
 */
class TerminalContextSaloha: public TerminalContext
{
 public:

	/**
	 * @brief  Create a terminal context.
	 *
	 * @param  tal_id          terminal id.
	 */
	TerminalContextSaloha(tal_id_t tal_id);
	~TerminalContextSaloha();

	/**
	 * @brief Add a new received packet in context and check if the PDU is complete
	 *
	 * @param packet  The Slotted Aloha Data packet
	 * @param pdu     OUT: A list of packets if the PDU is complete or an empty
	 *                     list if no PDU is completed
	 *
	 * @return no_prop  if no PDU can be propagated,
	 *         prop     if PDU can be propagated
	 */
	prop_state_t addPacket(SlottedAlohaPacketData *packet, saloha_packets_data_t &pdu);

  protected:

	typedef map<saloha_pdu_id_t, saloha_packets_data_t> pdus_t;
	/// The PDU fragments waiting to be propagated per QoS
	//  Fragments are propagated once all fragments of the complete PDU are received
	map<qos_t, pdus_t> wait_propagation;
	/// The oldest PDU ID per QoS in order to remove it after a certain amount of time
	map<qos_t, saloha_pdu_id_t> oldest_id;
	/// The counter for oldest packet
	saloha_pdu_id_t old_count;

	/// The slotted aloha logger
	OutputLog *log_saloha;

	/**
	 * @brief Handle oldest PDU id
	 *        If necessary remove old content and update oldest value
	 *
	 * @param qos         The current qos
	 */
	void handleOldest(qos_t qos, saloha_pdu_id_t current_id);

	/**
	 * @brief Find oldest pdu_id in waiting packets
	 *
	 * @param qos         The current qos
	 * @param current_id  The current PDU id
	 */
	void findOldest(qos_t qos);

};

#endif
