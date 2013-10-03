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

#ifndef BLOC_DVB_RCS_SAT_H
#define BLOC_DVB_RCS_SAT_H

#include <linux/param.h>

using namespace std;

#include "BlockDvb.h"
#include "SatSpot.h"

// output
#include <opensand_output/Output.h>

/**
 * Blocs heritate from mgl_bloc clam_singleSpot.sse
 * mgl_bloc classe defines some default handlers such as 'onEvent'
 */
class BlockDvbSat: public BlockDvb
{

 private:

	/// Whether the bloc has been initialized or not
	bool initOk;

	/// The satellite spots
	SpotMap spots;

	/// The satellite delay to emulate
	int m_delay;


	/* Timers */

	// Internal event handlers
	/// frame timer, used to awake the block regurlarly in order to send BBFrames
	event_id_t frame_timer;
	/// timer used to awake the block every second in order to retrieve
	/// the modcods
	event_id_t scenario_timer;

	/* misc */
	/// Flag set 1 to activate error generator
	// TODO remove?
	int m_useErrorGenerator;

	/// The terminal categories for forward band
	TerminalCategories categories;

	/// The terminal affectation for forward band
	TerminalMapping terminal_affectation;

	/// The default terminal category for forward band
	TerminalCategory *default_category;

	// TODO remove FMT groups from attributes
	/// FMT groups
	fmt_groups_t fmt_groups;

 public:

	BlockDvbSat(const string &name);
	~BlockDvbSat();

  protected:

	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);
	bool onInit();

 private:

	// initialization

	/**
	 * @brief Initialize the transmission mode
	 *
	 * @return  true on success, false otherwise
	 */
	bool initMode();

	/**
	 * @brief Initialize the error generator
	 *
	 * @return  true on success, false otherwise
	 */
	bool initErrorGenerator();

	/**
	 * @brief Read configuration for the different downward timers
	 *
	 * @return  true on success, false otherwise
	 */
	bool initDownwardTimers();

	/**
	 * Retrieves switching table entries
	 *
	 * @return  true on success, false otherwise
	 */
	bool initSwitchTable();

	/**
	 * @brief Retrieve the spots description from configuration
	 *
	 * @return  true on success, false otherwise
	 */
	bool initSpots();

	/**
	 * @brief Read configuration for the list of STs
	 *
	 * @return  true on success, false otherwise
	 */
	bool initStList();

	// event management
	//
	/**
	* Called upon reception event it is another layer (below on event) of demultiplexing
	* Do the appropriate treatment according to the type of the DVB message
	*
	* @param frame      the DVB or BB frame to forward
	* @param length     the length (in bytes) of the frame
	* @param carrier_id the carrier id of the frame
	* @return           true on success, false otherwise
	*/
	bool onRcvDvbFrame(unsigned char *frame,
	                   unsigned int length,
	                   unsigned int carrier_id);

	int sendSigFrames(DvbFifo * sigFifo);

	bool forwardDvbFrame(DvbFifo * sigFifo, char *ip_buf, int i_len);

	/**
	 * Send the DVB frames stored in the given MAC FIFO by
	 * @ref PhysicStd::onForwardFrame
	 *
	 * @param fifo          the MAC fifo which contains the DVB frames to send
	 * @param current_time  the current time
	 * @return              true on success, false otherwise
	 */
	bool onSendFrames(DvbFifo *fifo, long current_time);

	/**
	 * Forward a frame received by a transparent satellite to the
	 * given MAC FIFO (ef BlocDVBRcsSat::onSendFrames will extract it later)
	 *
	 * @param data_fifo     the MAC fifo to put the DVB frame in
	 * @param frame         the DVB frame to forward
	 * @param length        the length (in bytes) of the DVB frame to forward
	 * @param current_time  the current time
	 * @param fifo_delay    the minimum delay the DVB frame must stay in
	 *                      the MAC FIFO (used on SAT to emulate delay)
	 * @return              true on success, false otherwise
	 */
	virtual bool onForwardFrame(DvbFifo *data_fifo,
	                            unsigned char *frame,
	                            unsigned int length,
	                            long current_time,
	                            int fifo_delay);


	/**
	 * Get next random delay provided the two preceeding members
	 */
	inline int getNextDelay()
	{
		return this->m_delay;
	}

	/// update the probes
	void getProbe(NetBurst burst, DvbFifo fifo, spot_stats_t stat_fifo);
};

#endif
