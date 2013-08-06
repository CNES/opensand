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
#include "OpenSandFrames.h"
#include "MacFifoElement.h"
#include "FmtSimulation.h"

using std::string;

/**
 * @class PhysicStd
 * @brief Generic Physical Transmission Standard
 */
class PhysicStd
{

 private:

	/** The type of the DVB standard ("DVB-RCS" or "DVB-S2") */
	string type;

 protected:

	/** The real MODCOD of the ST */
	int realModcod;

	/** The received MODCOD */
	int receivedModcod;

    /** The packet representation */
	const EncapPlugin::EncapPacketHandler *packet_handler;

	/** The FMT simulated data */
	const FmtSimulation *fmt_simu;

	/** The frame duration */
	time_ms_t frame_duration_ms;

	/** The bandwidth */
	freq_khz_t bandwidth_khz;

	/** The remaining credit if all the frame duration was not consumed */
	time_ms_t remaining_credit_ms;

 public:

	/**
	 * Build a Physical Transmission Standard
	 *
	 * @param type           the type of the DVB standard
	 * @param packet_handler the packet handler
	 * @param fmt_simu       The simulated FMT data (only used for DVB-S2
	 *                                               output encapsulation scheme)
	 * @param frame_duration the frame duration (only used for DVB-S2
	 *                                           output encapsulation scheme)
	 * @param bandwidth      the bandwidth (only used for DVB-S2 output
	 *                                      encapsulation scheme)
	 */
	PhysicStd(string type,
	          const EncapPlugin::EncapPacketHandler *const pkt_hdl = NULL,
	          const FmtSimulation *const fmt_simu = NULL,
	          time_ms_t frame_duration_ms = 0,
	          freq_khz_t bandwidth_khz = 0);

	/**
	 * Destroy the Physical Transmission Standard
	 */
	virtual ~PhysicStd();

	/**
	 * Get the type of Physical Transmission Standard (DVB-RCS, DVB-S2, etc.)
	 *
	 * @return the type of Physical Transmission Standard
	 */
	string getType();


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
	 * sent by ef  BlocDvb::sendBursts
	 * @warning do not use this function on satellite terminal, use the dama
	 *          function ef DvbRcsDamaAgent::globalSchedule instead
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
	 * @brief Set the frame duration for DVB layer
	 */
	void setFrameDuration(int frame_duration);

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

};

#endif
