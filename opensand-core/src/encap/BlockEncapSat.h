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
 * @file BlockEncapSat.h
 * @brief Generic Encapsulation Bloc for SE
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef BLOC_ENCAP_SAT_H
#define BLOC_ENCAP_SAT_H

#include "OpenSandFrames.h"
#include "NetBurst.h"
#include "EncapPlugin.h"

#include <opensand_rt/Rt.h>
#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>


/**
 * @class BlockEncapSat
 * @brief Generic Encapsulation Bloc for SE
 */
class BlockEncapSat: public Block
{
 public:

	/**
	 * Build a satellite encapsulation bloc
	 *
	 * @param name      The name of the bloc
	 */
	BlockEncapSat(const string &name);

	/**
	 * Destroy the encapsulation bloc
	 */
	~BlockEncapSat();

	class Upward: public RtUpward
	{
	 public:
		Upward(Block *const bl):
			RtUpward(bl)
		{};
		bool onEvent(const RtEvent *const event);
	};

	class Downward: public RtDownward
	{
	 public:
		Downward(Block *const bl):
			RtDownward(bl)
		{};
		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 private:
		/// Expiration timers for encapsulation contexts
		std::map<event_id_t, int> timers;

		/// Output encapsulation context
		vector<EncapPlugin::EncapContext *> downlink_ctx;

		/**
		 * Handle a burst of encapsulation packets received from the lower-layer
		 * block
		 *
		 * @param burst  The burst received from the lower-layer block
		 * @return       Whether the burst was successful handled or not
		 */
		bool onRcvBurst(NetBurst *burst);

		/**
		 * Handle the timer event
		 *
		 * @param timer_id  The id of the timer to handle
		 * @return          Whether the timer event was successfully handled or not
		 */
		bool onTimer(event_id_t timer_id);

		/**
		 * Forward a burst of packets to the lower-layer block
		 *
		 * @param burst  The burst to forward
		 * @return       Whether the burst was successful forwarded or not
		 */
		bool ForwardPackets(NetBurst *burst);

		/**
		 * Encapsulate a burst of packets and forward the resulting
		 * burst of packets to the lower-layer block
		 *
		 * @param burst  The burst to encapsulate and forward
		 * @return       Whether the burst was successful encapsulated and forwarded
		 *               or not
		 */
		bool EncapsulatePackets(NetBurst *burst);
	};

 protected:

	/// event handlers
	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

	bool onInit();
};

#endif
