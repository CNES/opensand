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
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */


/**
 * @file ForwardSchedulingS2.h
 * @brief Scheduling for MAC FIFOs for DVB-S2 forward
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 *
 */

#ifndef _FORWARD_SCHEDULING_S2_H_
#define _FORWARD_SCHEDULING_S2_H_

#include "Scheduling.h"

#include "BBFrame.h"
#include "TerminalCategoryDama.h"


/** Status for the carrier capacity */
enum class sched_status
{
	ok,    // BBFrame added in the complete BBFrames list
	error, // Error when adding the BBFrame in the list
	full,  // The carrier is full, cannot add the BBFrame
};



/**
 * @class ForwardSchedulingS2
 * @brief Scheduling functions for MAC FIFOs with DVB-S2 forward
 */
class ForwardSchedulingS2: public Scheduling
{
public:
	ForwardSchedulingS2(time_us_t fwd_timer,
	                    EncapPlugin::EncapPacketHandler *packet_handler,
	                    const fifos_t &fifos,
	                    const StFmtSimuList *const fwd_sts,
	                    const FmtDefinitionTable *const fwd_modcod_def,
	                    const TerminalCategoryDama *const category,
	                    spot_id_t spot,
	                    bool is_gw,
	                    tal_id_t gw,
	                    std::string dst_name);

	virtual ~ForwardSchedulingS2();

	bool schedule(const time_sf_t current_superframe_sf,
	              std::list<Rt::Ptr<DvbFrame>> *complete_dvb_frames,
	              uint32_t &remaining_allocation) override;

protected:
	/** The timer for forward scheduling */
	time_us_t fwd_timer;

	/** the BBFrame being built identified by their modcod */
	std::map<unsigned int, Rt::Ptr<BBFrame>> incomplete_bb_frames;

	/** the BBframe being built in their created order (modcod only) */
	std::list<unsigned int> incomplete_bb_frames_ordered;

	/** the pending BBFrame list if there was not enough space in previous iteration
	 *  for the corresponding MODCOD */
	std::list<Rt::Ptr<BBFrame>> pending_bbframes;

	/** The FMT Definition Table associed */
	const FmtDefinitionTable *fwd_modcod_def;

	/** The terminal category */
	const TerminalCategoryDama *category;

	/// Spot Id
	spot_id_t spot_id;

	/// The section for probes used to name them
	std::string probe_section;

	// Total and unused capacity probes
	std::shared_ptr<Probe<int>> probe_fwd_total_capacity;
	std::shared_ptr<Probe<int>> probe_fwd_total_remaining_capacity;
	std::shared_ptr<Probe<int>> probe_bbframe_nbr;
	std::map<unsigned int, std::vector<std::shared_ptr<Probe<int> > > > probe_fwd_remaining_capacity;
	std::map<unsigned int, std::vector<std::shared_ptr<Probe<int> > > > probe_fwd_available_capacity;

	/// The MODCOD for emmited frames
	std::shared_ptr<Probe<int>> probe_gw_sent_modcod;

	/**
	 * @brief Schedule encapsulated packets from a FIFO and for a given Rs
	 *        The available capacity is obtained from carrier capacity in symbols
	 *
	 * @param fifo  The FIFO whee packets are stored
	 * @param current_superframe_sf  The current superframe number
	 * @param complete_dvb_frames    The list of complete DVB frames
	 * @param carriers               The carriers group
	 * @param capacity_sym           The capacity for this cycle, including previous space
	 * @param init_capa              The capacity for a whole cycle
	 */
	bool scheduleEncapPackets(DvbFifo *fifo,
	                          const time_sf_t current_superframe_sf,
	                          std::list<Rt::Ptr<DvbFrame>> *complete_dvb_frames,
	                          CarriersGroupDama *carriers,
	                          vol_sym_t &capacity_sym,
	                          vol_sym_t init_capa);

	sched_status schedulePacket(const time_sf_t current_superframe_sf,
                                unsigned int &sent_packets,
                                vol_sym_t &capacity_sym,
                                CarriersGroupDama *carriers,
                                std::list<Rt::Ptr<DvbFrame>> *complete_dvb_frames,
                                Rt::Ptr<NetPacket> encap_packet);

	/**
	 * @brief Create an incomplete BB frame
	 *
	 * @param bbframe   the BBFrame that will be created
	 * @param current_superframe_sf  The current superframe number
	 * @param modcod_id the BBFrame modcod
	 * @return          true on succes, false otherwise
	 */
	bool createIncompleteBBFrame(Rt::Ptr<BBFrame> &bbframe,
	                             const time_sf_t current_superframe_sf,
	                             unsigned int modcod_id);

	/**
	 * @brief Get the incomplete BBFrame for the current destination terminal
	 *
	 * @param tal_id    the terminal ID we want to send the frame
	 * @paarm carriers  the carriers group to which the terminal belongs
	 * @param current_superframe_sf  The current superframe number
	 * @param it OUT:   Iterator to the modcod and the incomplete BBframe for this packet
	 * @return          true on success, false otherwise
	 */
	bool prepareIncompleteBBFrame(tal_id_t tal_id,
	                              CarriersGroupDama *carriers,
	                              const time_sf_t current_superframe_sf,
	                              std::map<unsigned int, Rt::Ptr<BBFrame>>::iterator &it);

	/**
	 * @brief Add a BBframe to the list of complete BB frames
	 *
	 * @param complete_bb_frames the list of complete BB frames
	 * @param bbframe            the BBFrame to add in the list
	 * @param current_superframe_sf  The current superframe number
	 * @param duration_credit    IN/OUT: the remaining credit for the current frame
	 * @return                   sched_status::ok on success, sched_status::error on error and
	 *                           sched_status::full if there is not enough capacity
	 */
	sched_status addCompleteBBFrame(std::list<Rt::Ptr<DvbFrame>> *complete_bb_frames,
	                                Rt::Ptr<BBFrame>& bbframe,
	                                const time_sf_t current_superframe_sf,
	                                vol_sym_t &remaining_capacity_sym);


	/**
	 * @brief Schedule pending BBFrames from previous slot
	 *
	 * @param supported_modcods    The list of supported MODCODS for the current carrier
	 * @param current_superframe_sf  The current superframe number
	 * @param complete_dvb_frames  IN/OUT: The list of complete DVB frames
	 * @param capacity_sym         IN/OUT: The remaining capacity on carriers
	 */
	void schedulePending(const std::list<fmt_id_t> supported_modcods,
	                     const time_sf_t current_superframe_sf,
	                     std::list<Rt::Ptr<DvbFrame>> *complete_dvb_frames,
	                     vol_sym_t &remaining_capacity_sym);

	/**
	 * @brief  Get BBFrame size in symbols according to its MODCOD and size in bytes
	 *
	 * @param bbframe_size_bytes  The BBFrame size in bytes
	 * @param modcod_id           The BBFrame MODCOD ID
	 * @param current_superframe_sf  The current superframe number
	 * @param bbframe_size_sym    OUT: The BBFrame size in symbols
	 * @return true on success, false otherwise
	 */
	bool getBBFrameSizeSym(size_t bbframe_size_bytes,
	                       unsigned int modcod_id,
	                       const time_sf_t current_superframe_sf,
	                       vol_sym_t &bbframe_size_sym);

	/**
	 * @brief  Get BBFrame size in bytes according to its MODCOD
	 *
	 * @param modcod_id           The BBFrame MODCOD ID
	 * @return the BBFrame size in bytes
	 */
	unsigned int getBBFrameSizeBytes(unsigned int modcod_id);

	/**
	 * @brief  Create the associated probes
	 */
	void createProbes(CarriersGroupDama *vcm,
	                  std::vector<CarriersGroupDama *> vcm_carriers,
	                  std::vector<std::shared_ptr<Probe<int>>> &remain_probes,
	                  std::vector<std::shared_ptr<Probe<int>>> &avail_probes,
	                  unsigned int carriers_id);

	/**
	 * @brief  Check that the size of the carrier is compatible with the BBFrame size
	 */
	void checkBBFrameSize(CarriersGroupDama *vcm,
	                      std::vector<CarriersGroupDama *> vcm_carriers);
};

#endif
