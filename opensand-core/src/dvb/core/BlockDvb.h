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
 * @file BlockDvb.h
 * @brief This bloc implements a DVB-S2/RCS stack.
 * @author SatIP6
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 *
 *
 * <pre>
 *
 *            ^
 *            | encap burst
 *            v
 *    ------------------
 *   |                  |
 *   |       DVB        |
 *   |       Dama       |
 *   |                  |
 *    ------------------
 *            ^
 *            | DVB Frame / BBFrame
 *            v
 *
 * </pre>
 *
 */

#ifndef BLOCK_DVB_H
#define BLOCK_DVB_H

#include "PhysicStd.h"
#include "TerminalCategory.h"
#include "DvbChannel.h"

#include <opensand_rt/Rt.h>
#include <opensand_rt/RtChannel.h>


struct dvb_specific
{
	tal_id_t mac_id;
	spot_id_t spot_id;
	bool disable_control_plane;
	bool disable_acm_loop;
};


class BlockDvb: public Block
{
public:
	/**
	 * @brief DVB block constructor
	 *
	 */
	BlockDvb(const std::string &name);

	~BlockDvb();


	class DvbUpward: public DvbChannel, public RtUpward
	{
	public:
		DvbUpward(const std::string &name, dvb_specific specific);

		~DvbUpward();

	protected:
		virtual bool onRcvDvbFrame(DvbFrame *frame) = 0;

		/**
		 * Transmist a frame to the opposite channel
		 *
		 * @param frame The dvb frame
		 * @return true on success, false otherwise
		 */ 
		bool shareFrame(DvbFrame *frame);

		bool disable_control_plane;
		bool disable_acm_loop;
	};

	class DvbDownward: public DvbChannel, public RtDownward
	{
	public:
		DvbDownward(const std::string &name, dvb_specific specific);

		~DvbDownward();

	protected:
		/**
		 * @brief Read the common configuration parameters for downward channels
		 *
		 * @return true on success, false otherwise
		 */
		bool initDown(void);

		/**
		 * Receive Packet from upper layer
		 *
		 * @param packet        The encapsulation packet received
		 * @param fifo          The MAC FIFO to put the packet in
		 * @param fifo_delay    The minimum delay the packet must stay in the
		 *                      MAC FIFO (used on SAT to emulate delay)
		 * @return              true on success, false otherwise
		 */
		bool onRcvEncapPacket(std::unique_ptr<NetPacket> packet,
		                      DvbFifo *fifo,
		                      time_ms_t fifo_delay);

		/**
		 * Send the complete DVB frames created
		 * by \ref DvbRcsStd::scheduleEncapPackets or
		 * \ref DvbRcsDamaAgent::globalSchedule for Terminal
		 *
		 * @param complete_frames the list of complete DVB frames
		 * @param carrier_id      the ID of the carrier where to send the frames
		 * @return true on success, false otherwise
		 */
		bool sendBursts(std::list<DvbFrame *> *complete_frames,
		                uint8_t carrier_id);

		/**
		 * @brief Send message to lower layer with the given DVB frame
		 *
		 * @param frame       the DVB frame to put in the message
		 * @param carrier_id  the carrier ID used to send the message
		 * @return            true on success, false otherwise
		 */
		bool sendDvbFrame(DvbFrame *frame, uint8_t carrier_id);

		virtual bool handleDvbFrame(DvbFrame *frame) = 0;
		
		/**
		 * Update the statistics
		 */
		virtual void updateStats(void) = 0;

		bool disable_control_plane;
	};
};


#endif
