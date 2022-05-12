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
 * @file BlockDvbSat.h
 * @brief This bloc implements a DVB-S/RCS stack for a Satellite
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
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
#include "TerminalCategoryDama.h"
#include "SatGw.h"

#include <opensand_output/Output.h>


class PhysicStd;
class RtEvent;
class SatGw;


/// The map of satellite spots
typedef std::map<uint8_t, SatGw *> sat_gws_t;


class BlockDvbSat: public BlockDvb
{

 public:

	BlockDvbSat(const string &name);
	~BlockDvbSat();

	class Upward: public DvbUpward
	{
	 public:
		Upward(const string &name);

		~Upward();

		bool onInit(void);
		
		bool onEvent(const RtEvent *const event);
		
		void setGws(const sat_gws_t &gws);

	 protected: 
		/// reception standard (DVB-RCS or DVB-S2)      
		PhysicStd *reception_std; 
		
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
		
		/**
		 * @brief add st to the fmt simulation 
		 *
		 * @param current_gw The current SatGw
		 * @param st_id      The terminal id 
		 * @return true on success, false otherwise
		 */ 
		bool addSt(SatGw *current_gw, tal_id_t st_id);
		
		/**
		 * Handle corrupted frame
		 *
		 * @return true on success, false otherwise
		 */ 
		bool handleCorrupted(DvbFrame *dvb_frame);

		/**
		 * Handle Net Burst packet
		 * 
		 * @return true on success , false otherwise
		 */ 
		bool handleDvbBurst(DvbFrame *dvb_frame, SatGw *current_gw);

		/**
		 * Handle BB Frame
		 * 
		 * @return true on success, false otherwise
		 */ 
		bool handleBBFrame(DvbFrame *dvb_frame, SatGw *current_gw);
		
		/**
		 * Handle Saloha
		 *
		 * @return true on success, false otherwise
		 */ 
		bool handleSaloha(DvbFrame *dvb_frame, SatGw *current_gw);

		/// The satellite spots
		sat_gws_t gws;
	};

	class Downward: public DvbDownward
	{
	 public:
		Downward(const string &name);

		~Downward();
		
		bool onInit(void);
		
		bool onEvent(const RtEvent *const event);

		void setGws(const sat_gws_t &gws);

		bool handleDvbFrame(DvbFrame *frame);
	 
	 protected:
		/**
		 * Send the DVB frames stored in the given MAC FIFO
		 *
		 * @param fifo          the MAC fifo which contains the DVB frames to send
		 * @return              true on success, false otherwise
		 */
		bool sendFrames(DvbFifo *fifo);
				
		
		/**
		 * @brief handle event message
		 *
		 * @return true on success, false otherwise
		 */ 
		bool handleMessageBurst(const RtEvent *const event);
		
		/**
		 * @briel handle event timer
		 *
		 * @return true on success, false otherwise
		 */ 
		bool handleTimerEvent(SatGw *current_gw);

		/**
		 * @brief Initialize the link
		 *
		 * @return  true on success, false otherwise
		 */
		bool initSatLink(void);

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

		/// timer used to awake the block regurlarly in order to send frames
		//  and schedule in regenerative scenario
		event_id_t fwd_timer;

		/// The terminal affectation for forward band
		TerminalMapping<TerminalCategoryDama> terminal_affectation;

		/// The default terminal category for forward band
		TerminalCategoryDama *default_category;

		// TODO remove FMT groups from attributes
		/// FMT groups
		fmt_groups_t fmt_groups;

		/// The satellite spots
		sat_gws_t gws;

		/// The uplink C/N0 per terminal
		std::map<tal_id_t, double> cni;

		// Output probes and stats
		typedef std::map<unsigned int, std::shared_ptr<Probe<int> > > ProbeListPerSpot;

		// Frame interval
		std::shared_ptr<Probe<float>> probe_frame_interval;
	};


 protected:

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
	sat_gws_t gws;

};
#endif
