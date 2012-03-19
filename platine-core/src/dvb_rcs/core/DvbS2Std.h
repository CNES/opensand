/*
 *
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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

extern "C"
{
	#include <gse/constants.h>
	#include <gse/status.h>
	#include <gse/virtual_fragment.h>
	#include <gse/refrag.h>
}



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

	/** The incomplete BBFrame which is currently being completed */
	BBFrame *incomplete_bb_frame;

	/** The current MODCOD ID */
	unsigned int modcod_id;

	/** The current BBFrame duration */
	float bbframe_duration;

 public:

	/**
	 * Build a DVB-S2 Transmission Standard
	 */
	DvbS2Std();

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

	int scheduleEncapPackets(dvb_fifo *fifo,
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
	 * @param the name of the modcod definition file
	 * return 1 on success, 0 on error
	 */
	int loadModcodDefinitionFile(std::string filename);

	/**
	 * @brief Load the modcod simulation file
	 *
	 * @param the name of the modcod definition file
	 * return 1 on success, 0 on error
	 */
	int loadModcodSimulationFile(std::string filename);

	/**
	 * @brief Load the dra scheme definition file
	 *
	 * @param the name of the dra scheme definition file
	 * return 1 on success, 0 on error
	 */
	int loadDraSchemeDefinitionFile(std::string filename);

	/**
	 * @brief Load the dra scheme simulation file
	 *
	 * @param the name of the dra scheme definition file
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
	bool createOptionModcod(t_component comp, long pid,
	                        unsigned int modcod, long spot_id);
#endif

 private:

	/**
	 * @brief Get the current modcod of a terminl
	 *
	 * @param tal_id the terminal id
	 * @return       true on succes, false otherwise
	 */
	bool retrieveCurrentModcod(long tal_id);

	/**
	 * @brief Create an incomplete BB frame
	 *
	 * @return true on succes, false otherwise
	 */
	bool createIncompleteBBFrame();


	/**
	 * @brief Compute the duration of a BB frame encoded with the given MODCOD
	 *
	 * @return true if the MODCOD definition is found,
	 *         false otherwise
	 */
	bool getBBFRAMEDuration();

	/**
	 * @brief Initialize the current BB Frame parameter
	 *
	 * @param tal_id the terminal id we cant to send the frame
	 * @return       0 on success, -1 on error and -2 if the ST
	 *               is not logged in
	 */
	int initializeIncompleteBBFrame(unsigned int tal_id);

	/**
	 * @brief Process a MPEG packet that cannot be encapsulated
	 *        in the current BB frame
	 *
	 * @param complete_bb_frames the list of complete BB frames
	 * @param encap_packet       the packet got in the FIFO
	 * @param duration_credit    IN/OUT: the remaining credit for the current frame
	 * @param cpt_frame          IN/OUT: the number of completed frames
	 * @return                   0 on success, -1 on error and -2 if there is
	 *                           no more credit
	 */
	int processMpegPacket(std::list<DvbFrame *> *complete_bb_frames,
	                      NetPacket *encap_packet,
	                      float *duration_credit,
	                      unsigned int *cpt_frame);

	/**
	 * @brief Process a GSE packet that cannot be encapsulated
	 *        in the current BB frame
	 *
	 * @param complete_bb_frames the list of complete BB frames
	 * @param encap_packet       IN/OUT: the packet got in the FIFO
	 * @param duration_credit    IN/OUT: the remaining credit for the current frame
	 * @param cpt_frame          IN/OUT: the number of completed frames
	 * @param sent_packets       the number of packets encapsulated in the BB frame
	 * @param elem               the FIFO element
	 * @return                   0 on success, -1 on error and -2 if there is
	 *                           no more credit
	 */
	int processGsePacket(std::list<DvbFrame *> *complete_bb_frames,
	                     NetPacket **encap_packet,
	                     float *duration_credit,
	                     unsigned int *cpt_frame,
	                     unsigned int sent_packets,
	                     MacFifoElement *elem);

	/**
	 * @brief Add the current incomplete BB frame to the list of complete BB frames
	 *
	 * @param complete_bb_frames the list of complete BB frames
	 * @param duration_credit    IN/OUT: the remaining credit for the current frame
	 * @param cpt_frame          IN/OUT: the number of completed frames
	 * @return                   0 on success, -1 on error and -2 if there is
	 *                           no more credit
	 */
	int addCompleteBBFrame(std::list<DvbFrame *> *complete_bb_frames,
	                       float *duration_credit,
	                       unsigned int *cpt_frame);
};

#endif
