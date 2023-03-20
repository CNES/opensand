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
 * @file BLockDvbNcc.h
 * @brief This bloc implements a DVB-S/RCS stack for a Ncc.
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Bénédicte Motto <benedicte.motto@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 *
 *
 * <pre>
 *
 *
 *        |    encap   ^
 *        |    burst   |
 *        v            |
 *   +-----------------------+
 *   | downward  |   upward  |
 *   |           |           |
 *   | +-------+ | +-------+ |
 *   | | spots | | | spots | |
 *   | +-------+ | +-------+ |
 *    -----------+-----------+
 *        |            ^
 *        | DVB Frame  |
 *        v  BBFrame   |
 *
 * For spots description
 * @ref SpotDownward and @ref SpotUpward
 *
 * </pre>
 *
 */

#ifndef BLOCK_DVB_NCC_H
#define BLOCK_DVB_NCC_H

#include <opensand_rt/Block.h>
#include <opensand_rt/RtChannel.h>

#include "BlockDvb.h"

#include "NccPepInterface.h"
#include "NccSvnoInterface.h"
#include "DvbChannel.h"


class SpotDownward;
class SpotUpward;


template<>
class Rt::UpwardChannel<class BlockDvbNcc>: public DvbChannel, public Channels::Upward<UpwardChannel<BlockDvbNcc>>, public DvbFmt
{
 public:
	UpwardChannel(const std::string& name, dvb_specific specific);

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Event& event) override;
	bool onEvent(const MessageEvent& event) override;

 protected:
	/**
	 * Transmist a frame to the opposite channel
	 *
	 * @param frame The dvb frame
	 * @return true on success, false otherwise
	 */ 
	bool shareFrame(Ptr<DvbFrame> frame);

	/**
	 * @brief Initialize the output
	 *
	 * @return  true on success, false otherwise
	 */
	bool initOutput();
	
	bool onRcvDvbFrame(Ptr<DvbFrame> frame);

	/// the MAC ID of the ST (as specified in configuration)
	int mac_id;

	/// The id of the associated spot
	spot_id_t spot_id;

	std::unique_ptr<SpotUpward> spot;

	// log for slotted aloha
	std::shared_ptr<OutputLog> log_saloha;

	// Physical layer information
	std::shared_ptr<Probe<int>> probe_gw_received_modcod; // MODCOD of BBFrame received
	std::shared_ptr<Probe<int>> probe_gw_rejected_modcod; // MODCOD of BBFrame rejected

	bool disable_control_plane;
	bool disable_acm_loop;
};


template<>
class Rt::DownwardChannel<class BlockDvbNcc>: public DvbChannel, public Channels::Downward<DownwardChannel<BlockDvbNcc>>, public DvbFmt
{
 public:
	DownwardChannel(const std::string &name, dvb_specific specific);

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Event& event) override;
	bool onEvent(const TimerEvent& event) override;
	bool onEvent(const MessageEvent& event) override;
	bool onEvent(const NetSocketEvent& event) override;
	bool onEvent(const TcpListenEvent& event) override;

 protected:
	/**
	 * @brief Read the common configuration parameters for downward channels
	 *
	 * @return true on success, false otherwise
	 */
	bool initDown();

	/**
	 * Send the complete DVB frames created
	 * by \ref DvbRcsStd::scheduleEncapPackets or
	 * \ref DvbRcsDamaAgent::globalSchedule for Terminal
	 *
	 * @param complete_frames the list of complete DVB frames
	 * @param carrier_id      the ID of the carrier where to send the frames
	 * @return true on success, false otherwise
	 */
	bool sendBursts(std::list<Ptr<DvbFrame>> *complete_frames,
	                uint8_t carrier_id);

	/**
	 * @brief Send message to lower layer with the given DVB frame
	 *
	 * @param frame       the DVB frame to put in the message
	 * @param carrier_id  the carrier ID used to send the message
	 * @return            true on success, false otherwise
	 */
	bool sendDvbFrame(Ptr<DvbFrame> frame, uint8_t carrier_id);
	/**
	 * Read configuration for the downward timers
	 *
	 * @return  true on success, false otherwise
	 */
	bool initTimers();

	bool handleDvbFrame(Ptr<DvbFrame> frame);

	/**
	 * Send a Terminal Time Plan
	 */
	void sendTTP();

	/**
	 * Send a start of frame
	 */
	void sendSOF(unsigned int sof_carrier_id);

	/**
	 *  @brief Handle a logon request transmitted by the opposite
	 *         block
	 *
	 *  @param dvb_frame  The frame containing the logon request
	 *  @return true on success, false otherwise
	 */
	bool handleLogonReq(Ptr<DvbFrame> dvb_frame);

	/// The interface between Ncc and PEP
	NccPepInterface pep_interface;

	/// The interface between Ncc and SVNO
	NccSvnoInterface svno_interface;

	/// the MAC ID of the ST (as specified in configuration)
	tal_id_t mac_id;
	
	/// The id of the associated spot
	spot_id_t spot_id;

	bool disable_control_plane;

	/// counter for forward frames
	time_sf_t fwd_frame_counter;

	/// frame timer for return, used to awake the block every frame period
	event_id_t frame_timer;

	/// frame timer for forward, used to awake the block every frame period
	event_id_t fwd_timer;

	/// Delay for allocation requests from PEP (in ms)
	int pep_alloc_delay;

	std::unique_ptr<SpotDownward> spot;

	// Frame interval
	std::shared_ptr<Probe<float>> probe_frame_interval;
};


class BlockDvbNcc: public Rt::Block<BlockDvbNcc, dvb_specific>, public BlockDvb
{
public:
	/// Class constructor
	BlockDvbNcc(const std::string &name, struct dvb_specific specific);

	static void generateConfiguration(std::shared_ptr<OpenSANDConf::MetaParameter> disable_ctrl_plane);

protected:
	bool onInit() override;

	bool initListsSts();

	/// the MAC ID of the ST (as specified in configuration)
	int mac_id;

	/// The list of Sts with forward/down modcod for this spot
	std::shared_ptr<StFmtSimuList> output_sts;

	/// The list of Sts with return/up modcod for this spot
	std::shared_ptr<StFmtSimuList> input_sts;
};


#endif
