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
 * @file BlockDvb.h
 * @brief This bloc implements a DVB-S2/RCS stack.
 * @author SatIP6
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
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
#include "NccPepInterface.h"

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>


class BlockDvb: public Block
{

 public:

	/**
	 * @brief DVB block constructor
	 *
	 */
	BlockDvb(const string &name);

	~BlockDvb();


 protected:

	/**
	 * @brief Create a message with the given burst
	 *        and sned it to upper layer
	 *
	 * @param burst the burst of encapsulated packets
	 * @return  true on success, false otherwise
	 */
	bool SendNewMsgToUpperLayer(NetBurst *burst);

	// Common function for parameters reading
	bool initCommon();

	/**
	 * @brief Read configuration for the down/forward link MODCOD
	 *        definition/simulation files
	 *
	 * @return  true on success, false otherwise
	 */
	bool initForwardModcodFiles();

	/**
	 * @brief Read configuration for the up/return link MODCOD
	 *        definition/simulation files
	 *
	 * @return  true on success, false otherwise
	 */
	bool initReturnModcodFiles();

	/**
	 * Send the complete DVB frames created
	 * by ef DvbRcsStd::scheduleEncapPackets or
	 * \ ref DvbRcsDamaAgent::globalSchedule for Terminal
	 *
	 * @param complete_frames the list of complete DVB frames
	 * @param carrier_id      the ID of the carrier where to send the frames
	 * @return true on success, false otherwise
	 */
	bool sendBursts(std::list<DvbFrame *> *complete_frames, long carrier_id);

	// Send a DVB frame to the sat carrier block
	bool sendDvbFrame(T_DVB_HDR *dvb_frame, long carrier_id, long l_len);
	bool sendDvbFrame(DvbFrame *frame, long carrier_id);

	/// the satellite type (regenerative o transparent)
	sat_type_t satellite_type;

	/// the frame duration
	time_ms_t frame_duration_ms;

	/// the number of frame per superframe
	unsigned int frames_per_superframe;

	/// The MODCOD simulation elements
	FmtSimulation fmt_simu;

	/// the scenario refresh interval
	int dvb_scenario_refresh;

	/// The up/return link encapsulation packet
	EncapPlugin::EncapPacketHandler *up_return_pkt_hdl;

	/// The down/forward link encapsulation packet
	EncapPlugin::EncapPacketHandler *down_forward_pkt_hdl;

	/// output events
	static Event *error_init;
	static Event *event_login_received;
	static Event *event_login_response;

	/// emission standard (DVB-RCS or DVB-S2)
	PhysicStd *emissionStd;
	/// reception standard (DVB-RCS or DVB-S2)
	PhysicStd *receptionStd;
};

#endif
