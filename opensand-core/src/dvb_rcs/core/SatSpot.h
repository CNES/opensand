/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file SatSpot.h
 * @brief This bloc implements satellite spots
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#ifndef SAT_SPOT_H
#define SAT_SPOT_H

#include "DvbFifo.h"
#include "DvbFrame.h"
#include "Scheduling.h"
#include "FmtSimulation.h"
#include "TerminalCategory.h"

#include <sys/times.h>
#include <map>
#include <list>


using std::list;
using std::map;


/**
 * @class SatSpot
 * @brief A DVB-RCS/S2 spot for the satellite emulator
 */
class SatSpot
{

 private:

	uint8_t spot_id;            ///< Internal identifier of a spot

	uint8_t data_in_carrier_id; ///< the input carrier ID for the spot

	DvbFifo *control_fifo;     ///<  Fifo associated with Control carrier
	DvbFifo *logon_fifo;       ///<  Fifo associated with Logons
	DvbFifo *data_out_gw_fifo; ///<  Fifo associated with Data for the GW
	DvbFifo *data_out_st_fifo; ///<  Fifo associated with Data for the ST

	/// the list of complete DVB-RCS/BB frames that were not sent yet
	list<DvbFrame *> complete_dvb_frames;

	/// The downlink scheduling for regenerative satellite
	Scheduling *scheduling;

	// statistics

	/// Amount of layer 2 data received from ST
	vol_bytes_t l2_from_st_bytes;
	/// Amount of layer 2 data received from GW
	vol_bytes_t l2_from_gw_bytes;

	/// Mutex to protect access to spot element
	RtMutex spot_mutex;

 public:

	/**
	 * @brief Create spot
	 *
	 * @param spot_id            The spot id
	 * @param data_in_carrier_id The carrierid for incomming data
	 * @param log_id             The FIFO for logon packets
	 * @param ctrl_id            The FIFO for control frames
	 * @param data_out_st_id     The FIFO for outgoing terminal data
	 * @param data_out_gw_id     The FIFO for outgoing GW data
	 * @param fifo_size          The size of data FIFOs
	 */
	SatSpot(spot_id_t spot_id,
	        uint8_t data_in_carrier_id,
	        uint8_t log_id,
	        uint8_t ctrl_id,
	        uint8_t data_out_st_id,
	        uint8_t data_out_gw_id,
	        size_t fifo_size);
	~SatSpot();

	/**
	 * initialize the scheduling attribute
	 *
	 * @param pkt_hdl                The packet handler
	 * @param fwd_fmt_simu               The FMT simulation information
	 * @param category               The related terminal category
	 * @param frames_per_superframe  The number of frames per superframe
	 * @return true on success, false otherwise
	 */
	bool initScheduling(const EncapPlugin::EncapPacketHandler *pkt_hdl,
	                    FmtSimulation *const fwd_fmt_simu,
	                    const TerminalCategory *const category,
	                    unsigned int frames_per_superframe);

	/**
	 * @brief Schedule packets emission
	 *        Call scheduling @ref schedule function
	 *
	 * @param current_superframe_sf the current superframe (for logging)
	 * @param current_frame         the current frame
	 * @param current_time          the current time
	 *
	 * @return true on success, false otherwise
	 */
	bool schedule(const time_sf_t current_superframe_sf,
	              const time_frame_t current_frame,
	              clock_t current_time);


	/**
	 * @brief Get the spot ID
	 *
	 * @return the spot ID
	 */
	uint8_t getSpotId(void) const;

	/**
	 * @brief Get the input carrier ID
	 *
	 * @return the input carrier ID
	 */
	uint8_t getInputCarrierId(void) const;

	/**
	 * @brief Get the output data ST FIFO
	 *
	 * @return the output data ST FIFO
	 */
	DvbFifo *getDataOutStFifo(void) const;

	/**
	 * @brief Get the output data GW FIFO
	 *
	 * @return the output data GW FIFO
	 */
	DvbFifo *getDataOutGwFifo(void) const;

	/**
	 * @brief Get the control FIFO
	 *
	 * @return the control FIFO
	 */
	DvbFifo *getControlFifo(void) const;

	/**
	 * @brief Get the logon FIFO
	 *
	 * @return the logon FIFO
	 */
	DvbFifo *getLogonFifo(void) const;

	/**
	 * @brief Get the complete DVB frames list
	 *
	 * @return the list of complete DVB frames
	 */
	list<DvbFrame *> &getCompleteDvbFrames(void);

	/**
	 * @brief Update the amount of layer 2 data received from ST
	 *
	 * @param  bytes  The amount of layer 2 data received
	 */
	void updateL2FromSt(vol_bytes_t bytes);

	/**
	 * @brief Update the amount of layer 2 data received from GW
	 *
	 * @param  bytes  The amount of layer 2 data received
	 */
	void updateL2FromGw(vol_bytes_t bytes);

	/**
	 * @brief Get and reset the amount of layer 2 data received from ST
	 *
	 * @return  The amount of layer 2 data received
	 */
	vol_bytes_t getL2FromSt(void);

	/**
	 * @brief Get and reset the amount of layer 2 data received from GW
	 *
	 * @return  The amount of layer 2 data received
	 */
	vol_bytes_t getL2FromGw(void);
};


/// The map of satellite spots
typedef map<uint8_t, SatSpot *> sat_spots_t;

#endif
