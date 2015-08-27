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
 * @file FmtSimulation.h
 * @brief The FMT simulation elements
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef FMT_SIMULATION_H
#define FMT_SIMULATION_H

#include "OpenSandCore.h"

#include <opensand_output/OutputLog.h>

#include <vector>
#include <map>
#include <list>
#include <fstream>

using std::string;
using std::map;
using std::vector;
using std::list;
using std::ifstream;

// Be careful:
//  - both MODCODs definitions are used on DAMA controller to get the
//    needed information (Rs, SNR, ...) for allocation computation
//  - down/forward MODCOD definitions are also used on appropriate PhysicStd to get
//    the Frames size
//  - up/return MODCOD simulation ID is used on DVB-RCS up/return link,
//    we need the minimum supported MODCOD in order to choose the allocated
//    carrier in DAMA (needed by DamaCtrlRcs)
//  - down/forward MODCOD is used on DVB-S2 forward link on GW to get the minimum
//    supported MODCOD used in BBFrames (needed by DvbS2Std)
// Thus we instanciate this everywhere but only the GW and SAT instance may handle terminals


/**
 * @class FmtSimulation
 * @brief the FMT simulation elements
 */
class FmtSimulation
{
 private:

	/** The file stream for the MODCOD simulation file
	 *  Need pointer because ifstream is not copyable */
	ifstream *modcod_simu;

	/** Whether the MODCOD simulation file is defined or not */
	bool is_modcod_simu_defined;

	/** A list of the current MODCOD */
	vector<string> modcod_list;

	/** A list of the next MODCOD */
	vector<string> next_modcod_list;

	/** time of the next MODCOD change */
	double next_step;

	/** The ACM period refresh rate (ms) */
	time_ms_t acm_period_ms;


 protected:

	// Output log
	OutputLog *log_fmt;

 public:

	/**** constructor/destructor ****/

	/* create a list of Satellite Terminals (ST) */
	FmtSimulation();

	/* destroy a list of Satellite Terminals (ST) */
	~FmtSimulation();


	/**
	 * @brief Go to first step in adaptive physical layer scenario
	 *
	 * @return true on success, false otherwise
	 */
	bool goFirstScenarioStep();

	/**
	 * @brief Go to next step in adaptive physical layer scenario
	 *
	 * @param duration     the duration before the next_step
	 * @return true on success, false otherwise
	 */
	bool goNextScenarioStep(double &duration);

	/**
	 * @brief Set simulation file for MODCOD
	 *
	 * @param filename    the name of the file in which MODCOD scenario is described
	 * @param acm_period  the ACM period
	 * @return          true if the file exist and is valid, false otherwise
	 */
	bool setModcodSimu(const string &filename, time_ms_t acm_period_ms);


	/**
	 * @brief Get the is_modcod_simu_defined
	 *
	 * @return  is_modcod_simu_defined
	 */
	bool getIsModcodSimuDefined(void) const;

	/**
	 * @brief Get modcod_list
	 *
	 * @return  modcod_list
	 */
	vector<string> getModcodList(void) const;

	ifstream* getModcodSimu(void) const
	{
		return this->modcod_simu;
	}


 private:

	/**
	 * @brief Read a line of a simulation file and fill the MODCOD list
	 *
	 * @param   list      The MODCOD list of the next update
	 * @param   time      time of the next update
	 * @return            true on success, false on failure
	 *
	 * @todo better parsing
	 */
	bool setList(vector<string> &list,
	             double &time);

};

#endif
