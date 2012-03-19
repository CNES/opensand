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
 * @file SatSpot.h
 * @brief This bloc implements satellite spots
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#ifndef SAT_SPOT_H
#define SAT_SPOT_H


#include <sys/times.h>
#include <map>

using namespace std;

#include "dvb_fifo.h"
#include "DvbFrame.h"


typedef struct sat_statBloc
{
	long sum_data;
	clock_t previous_tick;
} sat_StatBloc;


/**
 * @class SatSpot
 * @brief A DVB-RCS/S2 spot for the satellite emulator
 *
 */
class SatSpot
{

 private:

	long m_spotId;            ///< Internal identifier of a spot

 public: // TODO: all the variables below should be private

	long m_dataInId;          ///< the input carrier ID for the spot

	dvb_fifo m_ctrlFifo;      ///<  Fifo associated with Control carrier
	dvb_fifo m_logonFifo;     ///<  Fifo associated with Logons
	dvb_fifo m_dataOutGwFifo; ///<  Fifo associated with Data for the GW
	dvb_fifo m_dataOutStFifo; ///<  Fifo associated with Data for the ST

	/// the list of complete DVB-RCS/BB frames that were not sent yet
	list<DvbFrame *> complete_dvb_frames;

	sat_StatBloc m_dataStat; ///< Used only with data FIFO, other are useless

 public:

	SatSpot();
	~SatSpot();

	int init(long spotId, long logId, long ctrlId,
	         long dataInId, long dataOutStId, long dataOutGwId);

	long getSpotId();

};


/// The map of satellite spots
typedef map<long, SatSpot *> SpotMap;

#endif
