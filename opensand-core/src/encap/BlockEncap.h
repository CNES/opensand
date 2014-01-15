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
 * @file BlockEncap.h
 * @brief Generic Encapsulation Bloc
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef BLOCK_ENCAP_H
#define BLOCK_ENCAP_H


#include "OpenSandFrames.h"
#include "NetPacket.h"
#include "NetBurst.h"
#include "StackPlugin.h"
#include "EncapPlugin.h"
#include "OpenSandCore.h"

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>
#include <opensand_conf/conf.h>

/**
 * @class BlockEncap
 * @brief Generic Encapsulation Bloc
 */
class BlockEncap: public Block
{
 private:

	/// Expiration timers for encapsulation contexts
	std::map<event_id_t, int> timers;

	/// it is the MAC layer group id received through msg_link_up
	long group_id;

	/// it is the MAC layer MAC id received through msg_link_up
	tal_id_t tal_id;

	/// State of the satellite link
	link_state_t state;

	/// the emission contexts list from lower to upper context
	std::vector<EncapPlugin::EncapContext *> emission_ctx;

	/// the reception contexts list from upper to lower context
	std::vector<EncapPlugin::EncapContext *> reception_ctx;

 public:

	/**
	 * Build an encapsulation block
	 *
	 * @param name  The name of the block
	 */
	BlockEncap(const string &name);

	/**
	 * Destroy the encapsulation bloc
	 */
	~BlockEncap();

 protected:

	// event handlers
	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

	// initialization method
	bool onInit();

 private:

	/**
	 * Handle the timer event
	 *
	 * @param timer_id  The id of the timer to handle
	 * @return          Whether the timer event was successfully handled or not
	 */
	bool onTimer(event_id_t timer_id);

	/**
	 * Handle a burst received from the upper-layer block
	 *
	 * @param burst  The burst received from the upper-layer block
	 * @return        Whether the IP packet was successful handled or not
	 */
	bool onRcvBurstFromUp(NetBurst *burst);

	/**
	 * Handle a burst of encapsulation packets received from the lower-layer
	 * block
	 *
	 * @param burst  The burst received from the lower-layer block
	 * @return       Whether the burst was successful handled or not
	 */
	bool onRcvBurstFromDown(NetBurst *burst);

	/// output events
	static Event *error_init;
};


#endif
