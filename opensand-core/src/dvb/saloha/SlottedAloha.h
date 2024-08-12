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
 * @file SlottedAloha.h
 * @brief The Slotted Aloha scheduling
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
*/

#ifndef SALOHA_H
#define SALOHA_H

#include "SimpleEncapPlugin.h"
#include "DvbFrame.h"
#include "SlottedAlohaPacket.h"
#include "SimpleEncapPlugin.h"
#include <opensand_output/Output.h>


/**
 * @class SlottedAloha
 * @brief The Slotted Aloha scheduling
*/
class SlottedAloha
{
protected:
	/// Number of superframe per Slotted Aloha Frame
	time_sf_t sf_per_saframe;

	/// The frame duration
	time_us_t frame_duration;

	/// Check whether the parent is correctly initialized
	bool is_parent_init;

	/// The encap packet handler
	SimpleEncapPlugin* pkt_hdl;

public:
	/**
	 * Build the slotted aloha class
	 */
	SlottedAloha();

	/**
	 * Class destructor
	 */
	virtual ~SlottedAloha();

	/**
	 * Init the Slotted Aloha parent class
	 *
	 * @param frame_duration         The frame duration
	 * @param pkt_hdl                The handler for encap packet
	 *
	 * @return true on success, false otherwise
	 */
	bool initParent(time_us_t frame_duration,
	                SimpleEncapPlugin*  pkt_hdl);

	/**
	 * Handle a received Slotted Aloha frame
	 *
	 * @param frame  The received frame
	 */
	virtual bool onRcvFrame(Rt::Ptr<DvbFrame> frame) = 0;

protected:
	/**
	 * Return check if current tick is a Slotted Aloha frame tick
	 *
	 * @param superframe_counter  counter of superfframes since the begining
	 *                            (the current superframe)
	 * @return true if current tick is a Slotted Aloha frame tick, false otherwise
	 */
	bool isSalohaFrameTick(time_sf_t superframe_counter);

	/// The slotted aloha logger
	std::shared_ptr<OutputLog> log_saloha;
	/// The init logger
	std::shared_ptr<OutputLog> log_init;

};

#endif

