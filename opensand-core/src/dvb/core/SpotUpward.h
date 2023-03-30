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
 * @file SpotUpward.h
 * @brief Upward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#ifndef SPOT_UPWARD_H
#define SPOT_UPWARD_H

#include <list>

#include "DvbChannel.h"


class SlottedAlohaNcc;
class StFmtSimuList;
class DvbRcs2Std;
class DvbScpcStd;
class NetBurst;


class SpotUpward: public DvbChannel, public DvbFmt
{
 public:
	SpotUpward(spot_id_t spot_id,
	           tal_id_t mac_id,
	           StackPlugin *upper_encap,
	           std::shared_ptr<StFmtSimuList> input_sts,
	           std::shared_ptr<StFmtSimuList> output_sts);

	~SpotUpward();

	static void generateConfiguration(std::shared_ptr<OpenSANDConf::MetaParameter> disable_ctrl_plane);

	/**
	 * @brief Spot Upward initialisation
	 *
	 * @return true on success, false otherwise
	 */ 
	bool onInit();


	/**
	 * @brief Handle a DVB frame
	 *
	 * @param frame  The frame
	 * @param burst  OUT: the burst of packets
	 * @return true on success, false otherwise
	 */
	bool handleFrame(Rt::Ptr<DvbFrame> frame, Rt::Ptr<NetBurst> &burst);

	/**
	 * @brief get CNI in a frame
	 *
	 * @param dvb_frame the Dvb Frame corrupted
	 */
	void handleFrameCni(DvbFrame& dvb_frame);

	/**
	 * @brief Schedule Slotted Aloha carriers
	 *
	 *	@param dvb_frame   a SoF
	 *  @param ack_frames  OUT: The generated ACK frames
	 *  @param sa_burst    OUT: The Slotted Aloha bursts received
	 *  @return true on success, false otherwise
	 */
	bool scheduleSaloha(Rt::Ptr<DvbFrame> dvb_frame,
	                    Rt::Ptr<std::list<Rt::Ptr<DvbFrame>>> &ack_frames,
	                    Rt::Ptr<NetBurst> &sa_burst);

	/**
	 *  @brief Handle a logon request transmitted by the lower layer
	 *
	 *  @param logon_req  The frame contining the logon request
	 *  @return true on success, false otherwise
	 */
	bool onRcvLogonReq(DvbFrame& dvb_frame);

	/**
	 *  @brief Handle a Slotted Aloha Data Frame
	 *
	 *  @param frame  The Slotted Aloha data frame
	 *  @return true on success, false otherwise
	 */
	bool handleSlottedAlohaFrame(Rt::Ptr<DvbFrame> frame);

	// statistics update
	void updateStats();

	/**
	 * @brief  handle a SAC frame
	 *
	 * @param dvb_frame The SAC frame
	 * @return true on success, false otherwise
	 */
	bool handleSac(DvbFrame &dvb_frame);

	/**
	 * @brief  Getter to spot_id
	 *
	 * @return spot_id
	 */
	inline uint8_t getSpotId() { return this->spot_id; }

	/**
	 * @brief Forward filter terminal ID to the encapsulation contexts
	 * 
	 * @param filter	The terminal ID used to filter packets on
	 */
	void setFilterTalId(tal_id_t filter) override;

 protected:
	/**
	 * @brief Initialize the transmission mode
	 *
	 * @return  true on success, false otherwise
	 */
	bool initMode();

	/**
	 * @brief Read configuration for the different files and open them
	 *
	 * @return  true on success, false otherwise
	 */
	bool initModcodSimu();

	/**
	 * @brief Initialize the ACM loop margins
	 *        Called in GW SpotUpward only as it will initialize
	 *        StFmtSimuList that are shared
	 *
	 * @return  true on success, false otherwise
	 */
	bool initAcmLoopMargin();

	/**
	 * @brief Initialize the statistics
	 *
	 * @return  true on success, false otherwise
	 */
	bool initOutput();

	/**
	 * Read configuration for the Slotted Aloha algorithm
	 *
	 * @return  true on success, false otherwise
	 */
	bool initSlottedAloha();

	/**
	 * Checks if SCPC mode is activated and configured
	 * (Available FIFOs and Carriers for SCPC)
	 *
	 * @return       Whether there are SCPC FIFOs and SCPC Carriers
	 *               available or not
	 */
	bool checkIfScpc();

	/// Spot Id
	spot_id_t spot_id;

	/// Gw tal id
	tal_id_t mac_id;

	/// The Slotted Aloha for NCC
	std::unique_ptr<SlottedAlohaNcc> saloha;

	/// reception standard (DVB-RCS or DVB-S2)
	std::unique_ptr<DvbRcs2Std> reception_std;

	/// reception standard for SCPC
	std::unique_ptr<DvbScpcStd> reception_std_scpc;

	/// The up/return packet handler for SCPC
	std::shared_ptr<EncapPlugin::EncapPacketHandler> scpc_pkt_hdl;
	encap_contexts_t scpc_ctx;

	/// FMT groups for up/return
	fmt_groups_t ret_fmt_groups;

	/// is terminal scpc map
	std::list<tal_id_t> is_tal_scpc;

	// Output probes and stats
	// Rates
	// Layer 2 from SAT
	std::shared_ptr<Probe<int>> probe_gw_l2_from_sat;
	int l2_from_sat_bytes;
	// Physical layer information
	std::shared_ptr<Probe<int>> probe_received_modcod;
	std::shared_ptr<Probe<int>> probe_rejected_modcod;

	/// log for slotted aloha
	std::shared_ptr<OutputLog> log_saloha;

	/// logon request events
	std::shared_ptr<OutputEvent> event_logon_req;
};


#endif
