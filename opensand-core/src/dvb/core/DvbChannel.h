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
 * @file DvbChannel.h
 * @brief A high level channel that implements some functions
 *        used by ST, SAT and/or GW
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 *
 */

#ifndef DVB_CHANNEL_H
#define DVB_CHANNEL_H

#include <sstream>

#include <opensand_rt/Ptr.h>

#include "StFmtSimu.h"
#include "DvbFrame.h"
#include "EncapPlugin.h"
#include "TerminalCategory.h"
#include "OpenSandModelConf.h"


class FifoElement;
class OutputLog;
class DvbFifo;


/**
 * @brief A high level channel that implements some functions
 *        used by ST, SAT and/or GW
 */
class DvbChannel
{
public:
	DvbChannel(StackPlugin *upper_encap, const std::string& name);

protected:
	/**
	 * @brief Read MODCOD Definition types
	 *
	 * @return true if success, false otherwise
	 */
	bool initModcodDefinitionTypes();

	/**
	 * @brief Read the encapsulation shcemes to get packet handler
	 *
	 * @param encap_schemes The section in configuration file for encapsulation
	 *                      schemes (up/return or down/forward)
	 * @param pkt_hdl       The packet handler corresponding to the encapsulation scheme
	 * @return true on success, false otherwise
	 */
	bool initPktHdl(EncapSchemeList encap_schemes,
	                std::shared_ptr<EncapPlugin::EncapPacketHandler> &pkt_hdl,
	                encap_contexts_t &ctx);

	/**
	 * @brief Forward filter terminal ID to the encapsulation contexts
	 * 
	 * @param filter	The terminal ID used to filter packets on
	 */
	virtual void setFilterTalId(tal_id_t filter);

	/**
	 * @brief Read the common configuration parameters
	 *
	 * @param encap_schemes The section in configuration file for encapsulation
	 *                      schemes (up/return or down/forward)
	 * @return true on success, false otherwise
	 */
	bool initCommon(EncapSchemeList encap_schemes);

	/**
	 * @brief Init the timer for statistics
	 *
	 * @param frame_duration  The frame duration that will be used to
	 *                        adujst the timer
	 */
	void initStatsTimer(time_us_t frame_duration);


	/**
	 * @brief init the band according to configuration
	 *
	 * @tparam  T The type of terminal category to create
	 * @param   spot                 The spot containing the section
	 * @param   section              The section in configuration file
	 *                               (up/return or down/forward)
	 * @param   access_type          The access type value
	 * @param   duration             The frame duration on this band
	 * @param   fmt_def              The MODCOD definition table
	 * @param   categories           OUT: The terminal categories
	 * @param   terminal_affectation OUT: The terminal affectation in categories
	 * @param   default_category     OUT: The default category if terminal is not
	 *                                    in terminal affectation
	 * @param   fmt_groups           OUT: The groups of FMT ids
	 * @return true on success, false otherwise
	 */
	template<class T>
	bool initBand(const OpenSandModelConf::spot &spot,
	              std::string section,
	              AccessType access_type,
	              time_us_t duration,
	              const FmtDefinitionTable &fmt_def,
	              TerminalCategories<T> &categories,
	              TerminalMapping<T> &terminal_affectation,
	              std::shared_ptr<T> &default_category,
	              fmt_groups_t &fmt_groups);

	/**
	 * @brief  Compute the bandplan.
	 *
	 * Compute available carrier frequency for each carriers group in each
	 * category, according to the current number of users in these groups.
	 *
	 * @tparam  T The type of terminal category to create
	 * @param   available_bandplan_khz  available bandplan (in kHz).
	 * @param   roll_off                roll-off factor
	 * @param   duration                The frame duration on this band
	 * @param   categories              pointer to category list.
	 *
	 * @return  true on success, false otherwise.
	 */
	template<class T>
	bool computeBandplan(freq_khz_t available_bandplan_khz,
	                     double roll_off,
	                     time_us_t duration,
	                     TerminalCategories<T> &categories);

	/**
	 * Receive Push data in FIFO
	 *
	 * @param fifo          The FIFO to put data in
	 * @param data          The data
	 * @param fifo_delay    The minimum delay the data must stay in the
	 *                      FIFO (used on SAT to emulate delay)
	 * @return              true on success, false otherwise
	 */
	bool pushInFifo(DvbFifo &fifo,
	                Rt::Ptr<NetContainer> data,
	                time_ms_t fifo_delay);

	/**
	 * @brief Whether it is time to send statistics or not
	 *
	 * @return true if statistics shoud be sent, false otherwise
	 */
	bool doSendStats();


	/**
	 * @brief   allocate more band to the demanding category
	 *
	 * @tparam  T The type of terminal category to create
	 * @param   duration             The frame duration on this band
	 * @param   cat_label            The label of the category
	 * @param   new_rate_kbps        The new rate for the category
	 * @param   categories           OUT: The terminal categories
	 * @return  true on success, false otherwise
	 */
	template<class T>
	bool allocateBand(time_us_t duration,
	                  std::string cat_label,
	                  rate_kbps_t new_rate_kbps,
	                  TerminalCategories<T> &categories);

	/**
	 * @brief   release band of the demanding category
	 *
	 * @tparam  T The type of terminal category to create
	 * @param   duration             The frame duration on this band
	 * @param   cat_label            The label of the category
	 * @param   new_rate_kbps        The new rate for the category
	 * @param   categories           OUT: The terminal categories
	 * @return  true on success, false otherwise
	 */
	template<class T>
	bool releaseBand(time_us_t duration,
	                 std::string cat_label,
	                 rate_kbps_t new_rate_kbps,
	                 TerminalCategories<T> &categories);

	/**
	 * @brief   Calculation of the carriers needed to be transfer from cat1 to cat2
	 *          in order to have a rate of new_rate_kbps on cat2
	 * @tparam  T The type of terminal category
	 * @param   cat           The category with to much carriers
	 * @param   rate_symps    The rate to be transfer (OUT: the surplus)
	 * @param   carriers      OUT: The informations about the carriers to be transfer
	 * @return  true on success, false otherwise
	 */
	template<class T>
	bool carriersTransferCalculation(std::shared_ptr<T> cat, rate_symps_t &rate_symps,
	                                 std::map<rate_symps_t, unsigned int> &carriers);

	/**
	 * @brief   Transfer of the carrier
	 *
	 * @tparam  T The type of terminal category
	 * @param   duration      The frame duration on this band
	 * @param   cat1          The category with to much carriers
	 * @param   cat2          The category with to less carriers
	 * @param   carriers      The informations about the carriers to be transfer
	 * @return  true on success, false otherwise
	 */
	template<class T>
	bool carriersTransfer(time_us_t duration,
	                      std::shared_ptr<T> cat1, std::shared_ptr<T> cat2,
	                      std::map<rate_symps_t , unsigned int> carriers);

	/// the RCS2 required burst length in symbol
	vol_b_t req_burst_length;

	/// the current super frame number
	time_sf_t super_frame_counter;

	/// the frame durations
	time_us_t fwd_down_frame_duration;
	time_us_t ret_up_frame_duration;

	/// The encapsulation packet handler
	std::shared_ptr<EncapPlugin::EncapPacketHandler> pkt_hdl;
	encap_contexts_t ctx;
	StackPlugin *upper_encap;

	/// The statistics period
	time_ms_t stats_period_ms;
	time_frame_t stats_period_frame;

	// log
	std::shared_ptr<OutputLog> log_init_channel;
	std::shared_ptr<OutputLog> log_receive_channel;
	std::shared_ptr<OutputLog> log_send_channel;

	static std::shared_ptr<OutputLog> dvb_fifo_log;

 private:
	/// Whether we can send stats or not (can send stats when 0)
	time_frame_t check_send_stats;
};


/**
 * @brief Some FMT functions for Dvb spots ou channels
 */
class DvbFmt
{
public:
	enum ModcodDefFileType
	{
		MODCOD_DEF_S2,
		MODCOD_DEF_RCS2,
	};

	DvbFmt();

	virtual ~DvbFmt() = default;

	/**
	 * @brief setter of input_sts
	 *
	 * @param the new input_sts
	 */
	void setInputSts(std::shared_ptr<StFmtSimuList> new_input_sts);

	/**
	 * @brief setter of output_sts
	 *
	 * @param the new output_sts
	 */
	void setOutputSts(std::shared_ptr<StFmtSimuList> new_output_sts);

	/**
	 * @brief Get the current MODCOD ID of the ST whose ID is given as input
	 *        for input list of sts
	 *
	 * @param id     the ID of the ST
	 * @return       the current MODCOD ID of the ST
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	uint8_t getCurrentModcodIdInput(tal_id_t id) const;

	/**
	 * @brief Get the current MODCOD ID of the ST whose ID is given as input
	 *        for output list of sts
	 *
	 * @param id     the ID of the ST
	 * @return       the current MODCOD ID of the ST
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	uint8_t getCurrentModcodIdOutput(tal_id_t id) const;

	/**
	 * @brief Get the CNI MODCOD ID of the ST whose ID is given as input
	 *        for output list of sts
	 *
	 * @param id     the ID of the ST
	 * @return       the CNI of the ST
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	double getRequiredCniInput(tal_id_t tal_id);

	/**
	 * @brief Get the CNI of the ST whose ID is given as input
	 *        for output list of sts
	 *
	 * @param id     the ID of the ST
	 * @return       the CNI of the ST
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	double getRequiredCniOutput(tal_id_t tal_id);

protected:
	/**
	 * @brief Read configuration for the MODCOD definition file and create the
	 *        FmtDefinitionTable class
	 *
	 * @param def               The section in configuration file for MODCOD definitions
	 *                          (up/return or down/forward)
	 * @param modcod_def        The FMT Definition Table attribute to initialize
	 * @param req_burst_length  The required burst length (only for DVB-RCS2)
	 * @return  true on success, false otherwise
	 */
	bool initModcodDefFile(ModcodDefFileType def, FmtDefinitionTable &modcod_def,
	                       vol_sym_t req_burst_length = 0);

	/**
	 * @brief Add a new Satellite Terminal (ST) in the output list
	 *
	 * @param id           the ID of the ST
	 * @param modcod_def   the MODCOD definition for the terminal on the input link
	 * @return             true if the addition is successful, false otherwise
	 */
	bool addOutputTerminal(tal_id_t id, const FmtDefinitionTable &modcod_def);

	/**
	 * @brief Add a new Satellite Terminal (ST) in the input list
	 *
	 * @param id           the ID of the ST
	 * @param modcod_def   the MODCOD definition for the terminal on the input link
	 * @return             true if the addition is successful, false otherwise
	 */
	bool addInputTerminal(tal_id_t id, const FmtDefinitionTable &modcod_def);

	/**
	 * @brief Delete a Satellite Terminal (ST) from the output list
	 *
	 * @param id      the ID of the ST
	 * @return    true if the deletion is successful, false otherwise
	 */
	bool delOutputTerminal(tal_id_t id);

	/**
	 * @brief Delete a Satellite Terminal (ST) from the input list
	 *
	 * @param id      the ID of the ST
	 * @return    true if the deletion is successful, false otherwise
	 */
	bool delInputTerminal(tal_id_t id);

	/**
	 * @brief Set the required  CNI for of the ST in input
	 *        whid ID is given as input according to the required Es/N0
	 *
	 * @param id               the ID of the ST
	 * @param cni              the required Es/N0 for that terminal
	 */
	void setRequiredCniInput(tal_id_t tal_id, double cni);

	/**
	 * @brief Set the required  Cni for of the ST in output
	 *        whid ID is given as input according to the required Es/N0
	 *
	 * @param id               the ID of the ST
	 * @param cni              the required Es/N0 for that terminal
	 */
	void setRequiredCniOutput(tal_id_t tal_id, double cni);

	/**
	 * @brief get the modcod change state
	 *
	 * @return the modcod change state
	 */
	bool getCniInputHasChanged(tal_id_t tal_id);

	/**
	 * @brief get the modcod change state
	 *
	 * @return the modcod change state
	 */
	bool getCniOutputHasChanged(tal_id_t tal_id);

	/**
	 * set extension to the packet GSE
	 *
	 * @param pkt_hdl         The GSE packet handler
	 * @param packet          The input packet
	 * @param source          The terminal source id
	 * @param dest            The terminal dest id
	 * @param extension_name  The name of the extension we need to add as
	 *                        declared in plugin
	 * @param super_frame_counter  The superframe counter (for debug messages)
	 * @param is_gw           Whether we are on GW or not
	 *
	 * @return The packet with extension on success, nullptr otherwise
	 */
	Rt::Ptr<NetPacket> setPacketExtension(std::shared_ptr<EncapPlugin::EncapPacketHandler> pkt_hdl,
	                                      Rt::Ptr<NetPacket> packet,
	                                      tal_id_t source,
	                                      tal_id_t dest,
	                                      std::string extension_name,
	                                      time_sf_t super_frame_counter,
	                                      bool is_gw);

	/** The internal map that stores all the STs and modcod id for input */
	std::shared_ptr<StFmtSimuList> input_sts;

	/// The MODCOD Definition Table for S2
	FmtDefinitionTable s2_modcod_def;

	/** The internal map that stores all the STs and modcod id for output */
	std::shared_ptr<StFmtSimuList> output_sts;

	/// The MODCOD Definition Table for RCS
	FmtDefinitionTable rcs_modcod_def;

	/// The ACM loop margin
	double fwd_down_acm_margin_db;
	double ret_up_acm_margin_db;

	// log
	std::shared_ptr<OutputLog> log_fmt;

private:
	/// Whether we can send stats or not (can send stats when 0)
	time_frame_t check_send_stats;

	/**
	 * @brief Delete a Satellite Terminal (ST) from the list
	 *
	 * @param id      the ID of the ST
	 * @param sts     the list which we have to delete a st
	 * @return    true if the deletion is successful, false otherwise
	 */
	bool delTerminal(tal_id_t id, StFmtSimuList &sts);
};


#endif
