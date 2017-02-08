/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 CNES
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
 * @file SatDelayMap.h
 * @brief Class containing all SatDelay plugins during the simulation
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.com>
 */

#ifndef SAT_DELAY_MAP_H
#define SAT_DELAY_MAP_H

#include "OpenSandCore.h"
#include "SatCarrierPlugin.h"

#include <opensand_rt/RtMutex.h>
#include <opensand_output/Output.h>

#include <string>
#include <stdint.h>


using std::string;

/**
 * @class SatDelayMap
 * @brief SatDelay map that contains plugins for all spots and gateways
 */
class SatDelayMap
{

 private:
	/* The map between carrier_id and the possible delay plugins */
	map<uint8_t, SatDelayPlugin**> carrier_delay;

	/* The map between spot_id and the plugin */
	map<spot_id_t, SatDelayPlugin*> spot_delay;

	/* The map between gw_id and the plugin */
	map<tal_id_t, SatDelayPlugin*> gw_delay;

	/* The refresh_period in miliseconds */
	time_ms_t refresh_period_ms;

	/* Output log */
	OutputLog *log_init;
	OutputLog *log_delay;

 public:
	/*
	 * @brief SatDelayMap constructor
	 */
	SatDelayMap();

	/*
	 * @brief SatDelayMap destructor
	 */
	~SatDelayMap();

	/*
	 * @brief initialize all maps
	 * @return true on success, false on error
	 */
	bool init();

	/*
	 * @brief update all sat delays
	 * @return true on success, false on error
	 */
	bool updateSatDelays();

	// TODO: - split ctrl carriers into ST<->SAT and GW<->SAT
	//       - merge getDelayIn and getDelayOut
	/*
	 * @brief get delay at the input of the satellite (from ST/GW to SAT)
	 * @param the carrier_id
	 * @param the dvb_frame type
	 * @param the delay
	 * @return true on sucess, false on error
	 */
	bool getDelayIn(uint8_t carrier_id, uint8_t msg_type, time_ms_t &delay);
	
	/*
	 * @brief get delay at the output of the satellite (from SAT to ST/GW)
	 * @param the carrier_id
	 * @param the dvb_frame type
	 * @param the delay
	 * @return true on sucess, false on error
	 */
	bool getDelayOut(uint8_t carrier_id, uint8_t msg_type, time_ms_t &delay);
	
	/*
	 * @brief get maximum possible delay between two terminals
	 * @param the delay
	 * @return true on sucess, false on error
	 */
	bool getMaxDelay(time_ms_t &delay);

	/*
	 * @brief get the refresh_period
	 */
	time_ms_t getRefreshPeriod() const { return this->refresh_period_ms; };
};


#endif
