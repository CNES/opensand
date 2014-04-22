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
#include "TerminalCategory.h"
#include "BBFrame.h"
#include "Sac.h"
#include "Ttp.h"

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>

class BlockDvbSat;
class BlockDvbNcc;
class BlockDvbTal;


class DvbChannel: public RtChannel
{
 public:
	DvbChannel(Block *const bl, chan_type_t chan):
		RtChannel(bl, chan),
		satellite_type(),
		with_phy_layer(false),
		super_frame_counter(0),
		pkt_hdl(NULL),
		stats_period_ms(),
		stats_timer(-1)
	{
	};

 protected:

	/**
	 * @brief Read the satellite type
	 *
	 * @return true on success, false otherwise
	 */
	bool initSatType(void);

	/**
	 * @brief Read the encapsulation shcemes to get packet handler
	 *
	 * @param encap_schemes The section in configuration file for encapsulation
	 *                      schemes (up/return or down/forward)
	 * @param pkt_hdl       The packet handler corresponding to the encapsulation scheme
	 * @return true on success, false otherwise
	 */
	bool initPktHdl(const char *encap_schemes,
	                EncapPlugin::EncapPacketHandler **pkt_hdl);


	/**
	 * @brief Read the common configuration parameters
	 *
	 * @param encap_schemes The section in configuration file for encapsulation
	 *                      schemes (up/return or down/forward)
	 * @return true on success, false otherwise
	 */
	bool initCommon(const char *encap_schemes);

	/// the satellite type (regenerative o transparent)
	sat_type_t satellite_type;

	/// Physical layer enable
	bool with_phy_layer;

	/// the current super frame number
	time_sf_t super_frame_counter;

	/// The encapsulation packet handler
	EncapPlugin::EncapPacketHandler *pkt_hdl;

	/// The statistics period
	time_ms_t stats_period_ms;

	/// Statistics timer
	event_id_t stats_timer;
};


class BlockDvb: public Block
{
 public:

	/**
	 * @brief DVB block constructor
	 *
	 */
	BlockDvb(const string &name):
		Block(name)
	{
		// register static logs
		BBFrame::bbframe_log = Output::registerLog(LEVEL_WARNING, "Dvb.Net.BBFrame");
		Sac::sac_log = Output::registerLog(LEVEL_WARNING, "Dvb.SAC");
		Ttp::ttp_log = Output::registerLog(LEVEL_WARNING, "Dvb.TTP");
	};


	~BlockDvb();

	class DvbUpward: public DvbChannel
	{
	 public:
		DvbUpward(Block *const bl):
			DvbChannel(bl, upward_chan),
			receptionStd(NULL)
		{};


		~DvbUpward();

	 protected:
		/// reception standard (DVB-RCS or DVB-S2)
		PhysicStd *receptionStd;
	};

	class DvbDownward: public DvbChannel
	{
	 public:
		DvbDownward(Block *const bl):
			DvbChannel(bl, downward_chan),
			fmt_simu(),
			frame_duration_ms(),
			fwd_timer_ms(),
			frames_per_superframe(-1),
			frame_counter(0),
			dvb_scenario_refresh(-1)
		{};


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
		bool onRcvEncapPacket(NetPacket *packet,
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
		bool sendBursts(list<DvbFrame *> *complete_frames,
		                uint8_t carrier_id);

		/**
		 * @brief Send message to lower layer with the given DVB frame
		 *
		 * @param frame       the DVB frame to put in the message
		 * @param carrier_id  the carrier ID used to send the message
		 * @return            true on success, false otherwise
		 */
		bool sendDvbFrame(DvbFrame *frame, uint8_t carrier_id);

		/**
		 * @brief init the band according to configuration
		 *
		 * @param band                 The section in configuration file
		 *                             (up/return or down/forward)
		 * @param duration_ms          The frame duration on this band
		 * @param categories           OUT: The terminal categories
		 * @param terminal_affectation OUT: The terminal affectation in categories
		 * @param default_category     OUT: The default category if terminal is not
		 *                                  in terminal affectation
		 * @param fmt_groups           OUT: The groups of FMT ids
		 * @return true on success, false otherwise
		 */
		bool initBand(const char *band,
		              time_ms_t duration_ms,
		              TerminalCategories &categories,
		              TerminalMapping &terminal_affectation,
		              TerminalCategory **default_category,
		              fmt_groups_t &fmt_groups);

		/**
		 * @brief  Compute the bandplan.
		 *
		 * Compute available carrier frequency for each carriers group in each
		 * category, according to the current number of users in these groups.
		 *
		 * @param   available_bandplan_khz  available bandplan (in kHz).
		 * @param   roll_off                roll-off factor
		 * @param   duration_ms             The frame duration on this band
		 * @param   categories              pointer to category list.
		 *
		 * @return  true on success, false otherwise.
		 */
		bool computeBandplan(freq_khz_t available_bandplan_khz,
		                     double roll_off,
		                     time_ms_t duration_ms,
		                     TerminalCategories &categories);


		/**
		 * @brief Read configuration for the MODCOD definition/simulation files
		 *
		 * @param def     The section in configuration file for MODCOD definitions
		 *                (up/return or down/forward)
		 * @param simu    The section in configuration file for MODCOD simulation
		 *                (up/return or down/forward)
		 * @return  true on success, false otherwise
		 */
		bool initModcodFiles(const char *def, const char *simu);

		/**
		 * @brief Read configuration for link MODCOD definition/simulation files
		 *
		 * @param def       The section in configuration file for MODCOD definitions
		 *                  (up/return or down/forward)
		 * @param simu      The section in configuration file for MODCOD simulation
		 *                  (up/return or down/forward)
		 * @param fmt_simu  The FMT simulation attribute to initialize
		 * @return  true on success, false otherwise
		 */
		bool initModcodFiles(const char *def, const char *simu,
		                     FmtSimulation &fmt_simu);

		/**
		 * Update the statistics
		 */
		virtual void updateStats(void) = 0;

	 protected:

		/// The MODCOD simulation elements
		FmtSimulation fmt_simu;
	
		/// the frame duration
		time_ms_t frame_duration_ms;

		/// the frame duration
		time_ms_t fwd_timer_ms;

		/// the number of frame per superframe
		unsigned int frames_per_superframe;

		/// the current frame number inside the current super frame
		time_frame_t frame_counter; // from 1 to frames_per_superframe

		/// the scenario refresh interval
		time_ms_t dvb_scenario_refresh;

		// Output log and debug
		OutputLog *log_band;
	};
};

#endif
