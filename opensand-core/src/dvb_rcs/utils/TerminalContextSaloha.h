/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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

#include "SlottedAlohaPacket.h"

#include <map>

using std::map;


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
	 * @brief Get the packet waiting to be propagated for the desired QoS
	 *
	 * @param qos  The QoS for the last propagated ID
	 * @return the packets waiting to be propagated
	 */
	saloha_packets_t *getWaitPropagationPackets(qos_t qos);

	/**
	 * @brief Get the last propagated IDs for the desired QoS
	 *
	 * @param qos  The QoS for the last propagated ID
	 * @return the last propagated ID
	 */
	saloha_id_t getLastPropagatedIds(qos_t qos);

	/**
	 * @brief Set the last propagated IDs for the desired QoS
	 *
	 * @param qos  The QoS for the last propagated ID
	 * @param id   The last propagated ID
	 */
	void setLastPropagatedIds(qos_t qos, saloha_id_t id);


  protected:

	/// The packets waiting to be propagated per QoS
	map<qos_t, saloha_packets_t *> wait_propagation;

	/// The IDs of last propagtated packets per QoS
	map<qos_t, saloha_id_t> last_propagated;

};

#endif
