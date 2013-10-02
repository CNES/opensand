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

typedef struct
{
	unsigned int sum_data;
	clock_t previous_tick;
} spot_stats_t;


/**
 * @class SatSpot
 * @brief A DVB-RCS/S2 spot for the satellite emulator
 */
class SatSpot
{

 private:

	uint8_t spot_id;          ///< Internal identifier of a spot

 public:

	unsigned int data_in_carrier_id; ///< the input carrier ID for the spot

	DvbFifo control_fifo;     ///<  Fifo associated with Control carrier
	DvbFifo logon_fifo;       ///<  Fifo associated with Logons
	DvbFifo data_out_gw_fifo; ///<  Fifo associated with Data for the GW
	DvbFifo data_out_st_fifo; ///<  Fifo associated with Data for the ST

	/// the list of complete DVB-RCS/BB frames that were not sent yet
	list<DvbFrame *> complete_dvb_frames;

	spot_stats_t data_stat; ///< Used only with data FIFO, other are useless

	/// The downlink scheduling for regenerative satellite
	Scheduling *scheduling;

 public:

	SatSpot();
	~SatSpot();

	/**
	 * @brief Initialize FIFOs for spot
	 *
	 * @param spot_id            The spot id
	 * @param data_in_carrier_id The carrierid for incomming data
	 * @param log_id             The FIFO for logon packets
	 * @param ctrl_id            The FIFO for control frames
	 * @param data_out_st_id     The FIFO for outgoing terminal data
	 * @param data_out_gw_id     The FIFO for outgoing GW data
	 * @param fifo_size          The size of data FIFOs
	 * @return true on success, false otherwise
	 */
	bool initFifos(spot_id_t spot_id,
	               unsigned int data_in_carrier_id,
	               unsigned int log_id,
	               unsigned int ctrl_id,
	               unsigned int data_out_st_id,
	               unsigned int data_out_gw_id,
	               size_t fifo_size);

	/**
	 * initialize the scheduling attribute
	 *
	 * @param pkt_hdl                The packet handler
	 * @param fmt_simu               The FMT simulation information
	 * @param category               The related terminal category
	 * @param frames_per_superframe  The number of frames per superframe
	 * @return true on success, false otherwise
	 */
	bool initScheduling(const EncapPlugin::EncapPacketHandler *pkt_hdl,
	                    FmtSimulation *const fmt_simu,
	                    const TerminalCategory *const category,
	                    unsigned int frames_per_superframe);


	/**
	 * @brief Get the spot ID
	 *
	 * @return the spot ID
	 */
	uint8_t getSpotId();

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


};


/// The map of satellite spots
typedef map<uint8_t, SatSpot *> SpotMap;

#endif
