/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
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
 * @file PhysicStd.h
 * @brief Generic Physical Transmission Standard
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien BERNARD / Viveris Technologies
 */

#ifndef PHYSIC_STD_H
#define PHYSIC_STD_H

#include <map>
#include <fstream>
#include <queue>

#include "DvbFifo.h"
#include "NetBurst.h"
#include "NetPacket.h"
#include "EncapPlugin.h"
#include "DvbFrame.h"
#include "msg_dvb_rcs.h"
#include "MacFifoElement.h"
#include "SatelliteTerminalList.h"
#include "DraSchemeDefinitionTable.h"


/**
 * @class PhysicStd
 * @brief Generic Physical Transmission Standard
 */
class PhysicStd
{

 private:

	/** The type of the DVB standard ("DVB-RCS" or "DVB-S2") */
	std::string _type;

 protected:

	/** The list of satellite terminals that are logged */
	SatelliteTerminalList satellite_terminals;

	/** The real MODCOD of the ST */
	int realModcod;

	/** The received MODCOD */
	int receivedModcod;

 protected:

    /** The packet representation */
	EncapPlugin::EncapPacketHandler *packet_handler;

	/** The frame duration */
	unsigned int frameDuration;

	/** The remaining credit if all the frame duration was not consumed */
	float remainingCredit;

	/** The bandwidth */
	int bandwidth;

 public: 

	/**
	 * Build a Physical Transmission Standard
	 *
	 * @param type           the type of the DVB standard
	 * @param packet_handler the packet handler
	 */
	PhysicStd(std::string type,
			  EncapPlugin::EncapPacketHandler *pkt_hdl = NULL);

	/**
	 * Destroy the Physical Transmission Standard
	 */
	virtual ~PhysicStd();

	/**
	 * Get the type of Physical Transmission Standard (DVB-RCS, DVB-S2, etc.)
	 *
	 * @return the type of Physical Transmission Standard
	 */
	std::string type();


	/***** functions for ST, GW and regenerative satellite *****/

	/**
	 * Receive Packet from upper layer
	 *
	 * @param packet        The encapsulation packet received
	 * @param fifo          The MAC FIFO to put the packet in
	 * @param current_time  The current time
	 * @param fifo_delay    The minimum delay the packet must stay in the
	 *                      MAC FIFO (used on SAT to emulate delay)
	 * @return              0 if succeed -1 otherwise
	 */
	virtual int onRcvEncapPacket(NetPacket *packet,
	                             DvbFifo *fifo,
	                             long current_time,
	                             int fifo_delay);

	/**
	 * Receive frame from lower layer and get the EncapPackets
	 *
	 * @param frame   the received DVB frame
	 * @param length  the length of the received DVB frame
	 * @param type    the type of the received DVB frame
	 * @param mac_id  the unsique MAC id of the terminal
	 *                (only used for DVB-S2)
	 * @param burst   OUT: a burst of encapsulation packets
	 * @return        0 if successful, -1 otherwise
	 */
	virtual int onRcvFrame(unsigned char *frame,
	                       long length,
	                       long type,
	                       int mac_id,
	                       NetBurst **burst) = 0;

	/**
	 * Schedule encapsulation packets and create DVB frames which will be
	 * sent by \ref  BlocDvb::sendBursts
	 * @warning do not use this function on satellite terminal, use the dama
	 *          function \ref DvbRcsDamaAgent::globalSchedule instead
	 *
	 * @param fifo                      the MAC fifo to get the packets from
	 * @param current_time              the current time
	 * @param complete_dvb_frames       the list of completed DVB frames
	 * @return                          0 if successful, -1 otherwise
	 */
	virtual int scheduleEncapPackets(DvbFifo *fifo,
	                                 long current_time,
	                                 std::list<DvbFrame *> *complete_dvb_frames) = 0;



	/***** functions for transparent satellite *****/

	/**
	 * Forward a frame received by a transparent satellite to the
	 * given MAC FIFO (\ref BlocDVBRcsSat::onSendFrames will extract it later)
	 *
	 * @param data_fifo     the MAC fifo to put the DVB frame in
	 * @param frame         the DVB frame to forward
	 * @param length        the length (in bytes) of the DVB frame to forward
	 * @param current_time  the current time
	 * @param fifo_delay    the minimum delay the DVB frame must stay in
	 *                      the MAC FIFO (used on SAT to emulate delay)
	 * @return              0 if successful, -1 otherwise
	 */
	virtual int onForwardFrame(DvbFifo *data_fifo,
	                           unsigned char *frame,
	                           unsigned int length,
	                           long current_time,
	                           int fifo_delay);

	/**
	 * @brief Set the frame duration for DVB layer
	 *        (only used for DVB-S2 output encapsulation scheme)
	 *
	 * @param frame_duration the frame duration
	 */
	void setFrameDuration(int frame_duration);

	/**
	 * @brief Set the bandwidth for DVB layer
	 *        (only used for DVB-S2 output encapsulation scheme)
	 *
	 * @param bandwidth the bandwidth
	 */
	void setBandwidth(int bandwidth);

	/**
	 * @brief Does a ST with the given ID exist ?
	 *
	 * @param id  the ID we want to check for
	 * @return    true if a ST, false is it does not exist
	 */
	bool doSatelliteTerminalExist(long id);

	/**
	 * @brief Delete a Satellite Terminal (ST) from the list
	 *
	 * @param id  the ID of the ST (called TAL ID or MAC ID elsewhere in the code)
	 * @return    true if the deletion is successful, false otherwise
	 */
	bool deleteSatelliteTerminal(long id);

	/**
	 * @brief Add a new Satellite Terminal (ST) in the list
	 *
	 * @param id               the ID of the ST (called TAL ID or MAC ID elsewhere
	 *                         in the code)
	 * @param simu_column_num  the column # associated to the ST for DRA/MODCOD
	 *                         simulation files
	 * @return                 true if the addition is successful, false otherwise
	 */
	bool addSatelliteTerminal(long id, unsigned long simu_column_num);

	/**
	 * @brief Go to next step in adaptive physical layer scenario
	 *
	 * Update current MODCOD and DRA scheme IDs of all STs in the list.
	 *
	 * @return true on success, false on failure
	 */
	bool goNextStScenarioStep();

	/**
	 * @brief Get the real MODCOD of the terminal
	 * 
	 * @return the real MODCOD
	 */
	int getRealModcod();

	/**
	 * @brief Get the received MODCOD of the terminal
	 * 
	 * @return the received MODCOD
	 */
	int getReceivedModcod();

	/**
	 * @brief Get the current DRA scheme ID of the ST whose ID is given as input
	 *
	 * @param id  the ID of the DRA scheme definition we want information for
	 * @return    the current DRA scheme ID of the ST
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	unsigned int getStCurrentDraSchemeId(long id);

};

#endif
