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
 * @file BLockDvbNcc.h
 * @brief This bloc implements a DVB-S/RCS stack for a Ncc.
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Bénédicte Motto <benedicte.motto@toulouse.viveris.com>
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
#define BLOCk_DVB_NCC_H

#include "BlockDvb.h"

#include "NccPepInterface.h"
#include "NccSvnoInterface.h"

#include "SpotUpward.h"
#include "SpotDownward.h"

/**
 * @brief  The list of spots for GW channels and other common elements
 */
class DvbSpotList
{
 public:

	DvbSpotList():
		spots(),
		default_spot(0),
		log_spot(NULL)
	{
		this->log_spot = Output::registerLog(LEVEL_WARNING, "Dvb.Spot");
	};

	virtual ~DvbSpotList()
	{
		map<spot_id_t, DvbChannel *>::iterator spot_iter;
		for(spot_iter = this->spots.begin(); 
		    spot_iter != this->spots.end(); ++spot_iter)
		{
			delete (*spot_iter).second;
		}
	};

	/**
	 * @brief Set the list of StFmtSimu for spots
	 *
	 * @param output_sts  The list of output sts per spot
	 * @param input_sts   The list of input sts per spot
	 */
	void setStFmt(const map<spot_id_t, StFmtSimuList *> &output_sts_list,
	              const map<spot_id_t, StFmtSimuList *> &input_sts_list);

 protected:

	/**
	 * @brief Create a spot list from configuration
	 *
	 * @return true on success, false otherwise
	 */
	bool initSpotList(void);

	/**
	 * @brief Get a spot with its spot_id
	 *
	 * @param spot_id  The spot id
	 * @return the spot if found, NULL otherwise
	 */
	DvbChannel *getSpot(spot_id_t spot_id) const;

	/**
	 * @brief Get the list of output StFmtSimu for a given spot
	 *
	 * @return the desired list of output StFmtSimu
	 */
	StFmtSimuList *getOutputStFmt(spot_id_t spot_id);

	/**
	 * @brief Get the list of input StFmtSimu for a given spot
	 *
	 * @return the desired list of input StFmtSimu
	 */
	StFmtSimuList *getInputStFmt(spot_id_t spot_id);

	/// The spots
	map<spot_id_t, DvbChannel *> spots;

	/// the default destination spot
	spot_id_t default_spot;

	/// The list of Sts with forward/down modcod per spot
	map<spot_id_t, StFmtSimuList *> output_sts_list;

	/// The list of Sts with return/up modcod per spot
	map<spot_id_t, StFmtSimuList *> input_sts_list;

 private:

	/**
	 * @brief Get the list of StFmtSimu for a given spot
	 *
	 * @return the desired list of StFmtSimu
	 */
	StFmtSimuList *getStFmt(spot_id_t spot,
	                        const map<spot_id_t, StFmtSimuList *> &sts);

	/// logging
	OutputLog *log_spot;
};





class BlockDvbNcc: public BlockDvb
{
 public:

	/// Class constructor
	BlockDvbNcc(const string &name, tal_id_t mac_id);

	~BlockDvbNcc();

	bool initListsSts();

	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);
	bool onInit();


	class Upward: public DvbUpward, public DvbSpotList
	{
	 public:
		Upward(Block *const bl, tal_id_t mac_id);
		~Upward();
		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 protected:
		/**
		 * Transmist a frame to the opposite channel
		 *
		 * @param frame The dvb frame
		 * @return true on success, false otherwise
		 */ 
		bool shareFrame(DvbFrame *frame);
		

		/// the MAC ID of the ST (as specified in configuration)
		int mac_id;

		// log for slotted aloha
		OutputLog *log_saloha;
	};


	class Downward: public DvbDownward, public DvbSpotList
	{
	  public:
		Downward(Block *const bl, tal_id_t mac_id);
		~Downward();
		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 protected:
		/**
		 * Read configuration for the downward timers
		 *
		 * @return  true on success, false otherwise
		 */
		bool initTimers(void);
		
		/**
		 * Send a Terminal Time Plan
		 */
		void sendTTP(SpotDownward *spot_downward);

		/**
		 * Send a start of frame
		 */
		void sendSOF(unsigned int sof_carrier_id);

		/**
		 *  @brief Handle a logon request transmitted by the opposite
		 *         block
		 *
		 *  @param dvb_frame  The frame contining the logon request
		 *  @param spot       The spot concerned by the request
		 *  @return true on success, false otherwise
		 */
		bool handleLogonReq(DvbFrame *dvb_frame,
		                    SpotDownward *spot);

		/**
		 * @brief Send a SAC message containing ACM parameters
		 *
		 * @return true on success, false otherwise
		 */
		bool sendAcmParameters(SpotDownward *spot_downward);
		
		// statistics update
		void updateStats(void);

		/// The interface between Ncc and PEP
		NccPepInterface pep_interface;
		
		/// The interface between Ncc and SVNO
		NccSvnoInterface svno_interface;

		/// the MAC ID of the ST (as specified in configuration)
		tal_id_t mac_id;

		/// counter for forward frames
		time_ms_t fwd_frame_counter;

		/// frame timer for return, used to awake the block every frame period
		event_id_t frame_timer;

		/// frame timer for forward, used to awake the block every frame period
		event_id_t fwd_timer;

		/// Delay for allocation requests from PEP (in ms)
		int pep_alloc_delay;

		// Frame interval
		Probe<float> *probe_frame_interval;
	};

 protected:

	/// The list of Sts with forward/down modcod per spot
	map<spot_id_t, StFmtSimuList *> output_sts_list;

	/// The list of Sts with return/up modcod per spot
	map<spot_id_t, StFmtSimuList *> input_sts_list;
};


#endif
