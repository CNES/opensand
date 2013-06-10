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
 * @file DvbS2Std.h
 * @brief DVB-S2 Transmission Standard
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef DVB_S2_STD_H
#define DVB_S2_STD_H

#include "PhysicStd.h"
#include "BBFrame.h"
#include "ModcodDefinitionTable.h"

/** The minimum duration required for a BB frame */
#define BBFRAME_MIN 1.8


/**
 * @class DvbS2Std
 * @brief DVB-S2 Transmission Standard
 */
class DvbS2Std: public PhysicStd
{

 private:
	/** The table of MODCOD definitions */
	ModcodDefinitionTable modcod_definitions;

	/** The table of DRA scheme definitions */
	DraSchemeDefinitionTable *dra_scheme_definitions;

	// the BBFrame being built identified by their modcod
	std::map<unsigned int, BBFrame *> incomplete_bb_frames;

	// the BBframe being built in their created order
	std::list<BBFrame *> incomplete_bb_frames_ordered;

	// the pending BBFrame if there was not enough space in previous iteration
	BBFrame *pending_bbframe;

 public:

	/**
	 * Build a DVB-S2 Transmission Standard
	 *
	 * @param packet_handler the packet handler
	 */
	DvbS2Std(EncapPlugin::EncapPacketHandler *pkt_hdl = NULL);

	/**
	 * Destroy the DVB-S2 Transmission Standard
	 */
	~DvbS2Std();

	/**
	 * @brief Get the payload size of a BBFRAME with the corresponding coding rate
	 *
	 * @param coding_rate  the corresponding coding rate
	 * @return             the payload in bytes
	 */
	static unsigned int getPayload(std::string coding_rate);

	int scheduleEncapPackets(DvbFifo *fifo,
	                         long current_time,
	                         std::list<DvbFrame *> *complete_dvb_frames);

	/* only for NCC and Terminals */
	int onRcvFrame(unsigned char *frame,
	               long length,
	               long type,
	               int mac_id,
	               NetBurst **burst);

	/**** MODCOD definition and scenario ****/

	/**
	 * @brief Load the modcod definition file
	 *
	 * @param filename the name of the modcod definition file
	 * return 1 on success, 0 on error
	 */
	int loadModcodDefinitionFile(std::string filename);

	/**
	 * @brief Load the modcod simulation file
	 *
	 * @param filename the name of the modcod definition file
	 * return 1 on success, 0 on error
	 */
	int loadModcodSimulationFile(std::string filename);

	/**
	 * @brief Load the dra scheme definition file
	 *
	 * @param filename the name of the dra scheme definition file
	 * return 1 on success, 0 on error
	 */
	int loadDraSchemeDefinitionFile(std::string filename);

	/**
	 * @brief Load the dra scheme simulation file
	 *
	 * @param filename the name of the dra scheme definition file
	 * return 1 on success, 0 on error
	 */
	int loadDraSchemeSimulationFile(std::string filename);

	/**
	 * @brief Get the dra scheme definitions
	 *
	 * @return the DRA scheme definitions
	 */
	DraSchemeDefinitionTable *getDraSchemeDefinitions();


#if 0 /* TODO: manage options */
	/**
	 * @brief Create real modcods option which is done when there is an update
	 *
	 * @param comp     the bloc component
	 * @param tal_id   the ID of the Satellite Terminal (ST)
	 * @param modcod   the modcod ID of destination terminal
	 * @param spot_id  the id of the spot of the destination terminal
	 * @return         true in case of success, false otherwise
	 */
	bool createOptionModcod(component_t comp, long pid,
	                        unsigned int modcod, long spot_id);
#endif

 private:

	/**
	 * @brief Get the current modcod of a terminal
	 *
	 * @param tal_id    the terminal id
	 * @param modcod_id OUT: the modcod_id retrived from the terminal ID
	 * @return          true on succes, false otherwise
	 */
	bool retrieveCurrentModcod(long tal_id, unsigned int &modcod_id);

	/**
	 * @brief Create an incomplete BB frame
	 *
	 * @param bbframe   the BBFrame that will be created
	 * @param modcod_id the BBFrame modcod
	 * @return          true on succes, false otherwise
	 */
	bool createIncompleteBBFrame(BBFrame **bbframe,
	                             unsigned int modcod_id);

	/**
	 * @brief Compute the duration of a BB frame encoded with the given MODCOD
	 *
	 * @param modcod_id        the modcod of the BBFrame
	 * @param bbframe_duration OUT: the BBFrame duration retrived from modcod ID
	 * @return                 true if the MODCOD definition is found,
 	 *                         false otherwise
	 */
	bool getBBFrameDuration(unsigned int modcod_id,
	                        float &bbframe_duration);

	/**
	 * @brief Get the incomplete BBFrame for the current destination terminal
	 *
	 * @param tal_id  the terminal ID we want to send the frame
	 * @param bbframe OUT: the BBframe for this packet
	 * @return        true on success, false otherwise
	 */
	bool getIncompleteBBFrame(unsigned int tal_id, BBFrame **bbframe);

	/**
	 * @brief Add a BBframe to the list of
	 *        complete BB frames
	 *
	 * @param complete_bb_frames the list of complete BB frames
	 * @param bbframe            the BBFrame to add in the list
	 * @param duration_credit    IN/OUT: the remaining credit for the current frame
	 * @return                   0 on success, -1 on error and -2 if there is
	 *                           not enough credit
	 */
	int addCompleteBBFrame(std::list<DvbFrame *> *complete_bb_frames,
	                       BBFrame *bbframe,
	                       float &duration_credit);
};

#endif
