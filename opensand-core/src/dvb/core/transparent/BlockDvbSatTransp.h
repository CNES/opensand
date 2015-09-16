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
 * @file BlockDvbSatTransp.h
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

#ifndef BLOC_DVB_SAT_TRANSP_H
#define BLOC_DVB_SAT_TRANSP_H


#include "BlockDvbSat.h"
#include "SatSpot.h"
#include "SatGw.h"
#include "PhysicStd.h" 

// output
#include <opensand_output/Output.h>

#include <linux/param.h>



class BlockDvbSatTransp: public BlockDvbSat
{

 public:

	BlockDvbSatTransp(const string &name);
	~BlockDvbSatTransp();

	class UpwardTransp: public Upward
	{
	 public:
		UpwardTransp(Block *const bl);
		~UpwardTransp();
		

	 private:
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
		* handle corrupted frame
		*
		* @param dvb_frame  the DVB or BB frame to forward
		* @return           true on success, false otherwise
		*/
		bool handleCorrupted(DvbFrame *dvb_frame);
		
		/**
		 * Handle Net Burst packet
		 * 
		 * @return true on success , false otherwise
		 */ 
		bool handleDvbBurst(DvbFrame *dvb_frame,
		                    SatGw *current_gw,
		                    SatSpot *current_spot);
		
		/**
		 * Handle Sac
		 * this function do something only on regenerative mode
		 * 
		 * @return true on success, false otherwise
		 */ 
		bool handleSac(DvbFrame *dvb_frame,
		               SatGw *current_gw);
		
		/**
		 * Handle BB Frame
		 * 
		 * @return true on success, false otherwise
		 */ 
		bool handleBBFrame(DvbFrame *dvb_frame, 
		                   SatGw *current_gw,
		                   SatSpot *current_spot);
		
		/**
		 * Handle Saloha
		 *
		 * @return true on success, false otherwise
		 */
		bool handleSaloha(DvbFrame *dvb_frame, 
		                  SatGw *current_gw,
		                  SatSpot *current_spot);

		/**
		 * @brief  Add a new line in the MODCOD time series generator file
		 *
		 * @return  true on success, false otherwise
		 */
		bool updateSeriesGenerator(void);

	};

	class DownwardTransp: public Downward
	{
	 public:
		DownwardTransp(Block *const bl);
		~DownwardTransp();
		
	 private:
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
		bool handleTimerEvent(SatGw *current_gw,
		                      uint8_t spot_id);
		
		/**
		 * @ brief handle scenario event timer
		 * This fonction is never called in Transparent mode
		 *
		 * @return true on success, false otherwise
		 */ 
		bool handleScenarioTimer(SatGw *current_gw);

	};


  protected:

	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

};
#endif
