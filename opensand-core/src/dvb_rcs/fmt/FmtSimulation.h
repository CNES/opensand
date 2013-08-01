/*
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
 * @file FmtSimulation.h
 * @brief The FMT simulation elements
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef FMT_SIMULATION_H
#define FMT_SIMULATION_H

#include "StFmtSimu.h"
#include "OpenSandCore.h"
#include "FmtDefinitionTable.h"

#include <vector>
#include <map>
#include <fstream>

using std::string;
using std::map;
using std::vector;
using std::ifstream;

// TODO rename MODCOD and DRA forward/return ?

// Be careful:
//  - MODCOD and DRA definitions are used on DAMA controller to get the
//    needed information (Rs, SNR, ...) for allocation computation
//  - MODCOD definitions are also used on appropriate PhysicStd to get
//    the Frames  size
//  - DRA simulation ID is used on DVB-RCS up/return link, we need the minimum
//    supported DRA in order to choose the allocated carrier in DAMA (needed by DamaCtrlRcs)
//  - MODCOD is used on DVB-S2 forward link on GW to get the minimum supported MODCOD
//    used in BBFrames (needed by DvbS2Std)
// Thus we instanciate this everywhere but only the GW and SAT instance may handle terminals

/**
 * @class FmtSimulation
 * @brief The FMT simulation elements
 */
class FmtSimulation
{
 private:

	/** The internal map that stores all the STs */
	map<tal_id_t, StFmtSimu *> sts;

	/** The table of MODCOD definitions */
	FmtDefinitionTable modcod_definitions;

	/** The file stream for the MODCOD simulation file */
	ifstream modcod_simu_file;

	/** The table of DRA definitions */
	FmtDefinitionTable dra_scheme_definitions;

	/** The file stream for the DRA scheme simulation file */
	ifstream dra_scheme_simu_file;

	/** Whether the MODCOD simulation file is defined or not */
	bool is_modcod_simu_file_defined;

	/** Whether the DRA scheme simulation file is defined or not */
	bool is_dra_scheme_simu_file_defined;

	/** A list of the current MODCOD */
	vector<string> modcod_list;

	/** A list of the current DRA */
	vector<string> dra_list;

 public:

	/**** constructor/destructor ****/

	/* create a list of Satellite Terminals (ST) */
	FmtSimulation();

	/* destroy a list of Satellite Terminals (ST) */
	~FmtSimulation();


	/* add a new Satellite Terminal (ST) in the list */
	bool addTerminal(tal_id_t id,
	                 unsigned long simu_column_num);

	/* delete a Satellite Terminal (ST) from the list */
	bool delTerminal(tal_id_t id);

	/* does a ST with the given ID exist ? */
	bool doTerminalExist(tal_id_t id) const;

	/* clear the list of STs */
	void clear();

	/* go to next step in adaptive physical layer scenario */
	bool goNextScenarioStep();

	/* was the current MODCOD IDs of all the STs
	   advertised over the emulated network ? */
	bool areCurrentModcodsAdvertised();

	/**
	 * @brief Set definition file for MODCOD
	 *
	 * @param filename The MODCOD definition file
	 * @return true on success, false otherwise
	 */
	bool setModcodDefFile(string filename);

	/* set simulation file for MODCOD */
	bool setModcodSimuFile(string filename);

	/**
	 * @brief Set definition file for DRA
	 *
	 * @param filename The DRA definition file
	 * @return true on success, false otherwise
	 */
	bool setDraSchemeDefFile(string filename);

	/* set simulation file for DRA scheme */
	bool setDraSchemeSimuFile(string filename);

	/* get the column # associated to the ST
	   whose ID is given as input */
	unsigned int getSimuColumnNum(tal_id_t id) const;

	/* get the current MODCOD ID of the ST
	   whose ID is given as input */
	unsigned int getCurrentModcodId(tal_id_t id) const;

	/* get the previous MODCOD ID of the ST
	   whose ID is given as input */
	unsigned int getPreviousModcodId(tal_id_t id) const;

	/* was the current MODCOD ID of the ST whose ID is given as input
	   advertised over the emulated network ? */
	bool isCurrentModcodAdvertised(tal_id_t id) const;

	/* get the current DRA scheme ID of the ST
	   whose ID is given as input */
	unsigned int getCurrentDraSchemeId(tal_id_t id) const;

	/*  get the terminal ID corresponding to the lower modcod */
	tal_id_t getTalIdCorrespondingToLowerModcod() const;

	/**
	 * @brief Get the MODCOD definitions
	 *
	 * @return the MODCOD definitions
	 */
	const FmtDefinitionTable *getModcodDefinitions() const;

	/**
	 * @brief Get the DRA definitions
	 *
	 * @return the DAR definitions
	 */
	const FmtDefinitionTable *getDraSchemeDefinitions() const;

 private:

	/* update the current MODCOD IDs of all STs from MODCOD simulation file */
	bool goNextScenarioStepModcod();

	/* update the current DRA scheme IDs of all STs from DRA simulation file */
	bool goNextScenarioStepDraScheme();

	/* read the next line of the simulation file
	 * and store it in the appropriate list */
	bool setList(ifstream &simu_file,
	             vector<string> &list);

};

#endif
