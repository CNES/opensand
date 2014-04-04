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
 * @file BlockDvbSat.h
 * @brief This bloc implements a DVB-S/RCS stack for a Satellite
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 * <pre>
 *
 *                  ^
 *                  | DVB Frame / BBFrame
 *                  v
 *           ------------------
 *          |                  |
 *          |  DVB-RCS Sat     |  <- Set carrier infos
 *          |                  |
 *           ------------------
 *
 * </pre>
 *
 */

#ifndef BLOC_DVB_SAT_H
#define BLOC_DVB_SAT_H


#include "BlockDvb.h"
#include "SatSpot.h"

// output
#include <opensand_output/Output.h>

#include <linux/param.h>

/**
 * Blocs heritate from mgl_bloc clam_singleSpot.sse
 * mgl_bloc classe defines some default handlers such as 'onEvent'
 */
class BlockDvbSat: public BlockDvb
{

 public:

	BlockDvbSat(const string &name);
	~BlockDvbSat();

	class Upward: public DvbUpward
	{
	 public:
		Upward(Block *const bl);

		~Upward();
		bool onInit(void);
		bool onEvent(const RtEvent *const event);
		void setSpots(const sat_spots_t &spots);

	 private:
		/**
		* Called upon reception event it is another layer (below on event) of demultiplexing
		* Do the appropriate treatment according to the type of the DVB message
		*
		* @param dvb_frame  the DVB or BB frame to forward
		* @return           true on success, false otherwise
		*/
		bool onRcvDvbFrame(DvbFrame *dvb_frame);

		/**
		 * Forward a frame received by a transparent satellite to the
		 * given MAC FIFO (\ref BlocDvbSat::sendFrames will extract it later)
		 *
		 * @param fifo       The FIFO to put the frame in
		 * @param dvb_frame  The frame to forward
		 * @return true on success, false otherwise
		 */
		bool forwardDvbFrame(DvbFifo *fifo, DvbFrame *dvb_frame);

		/**
		 * @brief Initialize the transmission mode
		 *
		 * @return  true on success, false otherwise
		 */
		bool initMode(void);

		/**
		 * Retrieves switching table entries
		 *
		 * @return  true on success, false otherwise
		 */
		bool initSwitchTable(void);

		/// The satellite spots
		sat_spots_t spots;

		/// The uplink C/N0 per terminal
		map<tal_id_t, double> cni;

		/// The satellite delay to emulate
		time_ms_t sat_delay;
	};

	class Downward: public DvbDownward
	{
	 public:
		Downward(Block *const bl);

		~Downward();
		bool onInit(void);
		bool onEvent(const RtEvent *const event);
		void setSpots(const sat_spots_t &spots);

	 private:
		/**
		 * Send the DVB frames stored in the given MAC FIFO
		 *
		 * @param fifo          the MAC fifo which contains the DVB frames to send
		 * @return              true on success, false otherwise
		 */
		bool sendFrames(DvbFifo *fifo);

		/**
		 * Send the DVB sig frames
		 *
		 * @param frame  the sig frame
		 * @param spot   the spot on which the frame is sent
		 * @return       true on success, false otherwise
		 */
		bool sendSigFrame(DvbFrame *frame, const SatSpot *const spot);

		/**
		 * @brief Initialize the link
		 *
		 * @return  true on success, false otherwise
		 */
		bool initSatLink(void);

		/**
		 * @brief Read configuration for the list of STs
		 *
		 * @return  true on success, false otherwise
		 */
		bool initStList(void);

		/**
		 * @brief Read configuration for the different timers
		 *
		 * @return  true on success, false otherwise
		 */
		bool initTimers(void);

		/**
		 * @brief Initialize the statistics part
		 *
		 * @return true on success, false otherwise
		 */
		bool initOutput(void);

		/**
		 * Update the statistics on the satellite
		 */
		void updateStats(void);

		/// the counter for downlink frames
		time_sf_t down_frame_counter;

		/// The satellite delay to emulate
		time_ms_t sat_delay;

		/// timer used to awake the block regurlarly in order to send frames
		//  and schedule in regenerative scenario
		event_id_t fwd_timer;

		/// timer used to awake the block every second in order to retrieve
		/// the modcods
		event_id_t scenario_timer;

		/// The terminal categories for forward band
		TerminalCategories categories;

		/// The terminal affectation for forward band
		TerminalMapping terminal_affectation;

		/// The default terminal category for forward band
		TerminalCategory *default_category;

		// TODO remove FMT groups from attributes
		/// FMT groups
		fmt_groups_t fmt_groups;

		/// The satellite spots
		sat_spots_t spots;

		// Output probes and stats
		typedef map<unsigned int, Probe<int> *> ProbeListPerSpot;

			// Queue sizes
		ProbeListPerSpot probe_sat_output_gw_queue_size;
		ProbeListPerSpot probe_sat_output_gw_queue_size_kb;
		ProbeListPerSpot probe_sat_output_st_queue_size;
		ProbeListPerSpot probe_sat_output_st_queue_size_kb;
			// Rates
		ProbeListPerSpot probe_sat_l2_from_st;
		ProbeListPerSpot probe_sat_l2_to_st;
		ProbeListPerSpot probe_sat_l2_from_gw;
		ProbeListPerSpot probe_sat_l2_to_gw;
			// Frame interval
		Probe<float> *probe_frame_interval;
	};


  protected:

	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);
	bool onInit();


 private:

	// initialization

	/**
	 * @brief Retrieve the spots description from configuration
	 *
	 * @return  true on success, false otherwise
	 */
	bool initSpots(void);

	/// The satellite spots
	//  We keep them here for release in desctructor,
	//  they are also used in upward and downward, be careful
	sat_spots_t spots;


};
#endif
