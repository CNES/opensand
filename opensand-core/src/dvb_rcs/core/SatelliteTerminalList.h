/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 * @file SatelliteTerminalList.h
 * @brief A list of Satellite Terminals (ST)
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef SATELLITE_TERMINAL_LIST_H
#define SATELLITE_TERMINAL_LIST_H

#include "SatelliteTerminal.h"
#include <opensand_conf/conf.h>

#include <map>
#include <fstream>


/**
 * @class SatelliteTerminalList
 * @brief A list of Satellite Terminals (ST)
 */
class SatelliteTerminalList
{
 private:

	/** The internal map that stores all the STs */
	std::map<long, SatelliteTerminal *> sts;

	/** The file stream for the MODCOD simulation file */
	std::ifstream modcod_simu_file;

	/** Whether the MODCOD simulation file is defined or not */
	bool is_modcod_simu_file_defined;

	/** The file stream for the DRA scheme simulation file */
	std::ifstream dra_scheme_simu_file;

	/** Whether the DRA scheme simulation file is defined or not */
	bool is_dra_scheme_simu_file_defined;

	/** A list of the current MODCOD */
	std::vector<std::string> modcod_list;

	/** A list of the current DRA */
	std::vector<std::string> dra_list;

 public:

	/**** constructor/destructor ****/

	/* create a list of Satellite Terminals (ST) */
	SatelliteTerminalList();

	/* destroy a list of Satellite Terminals (ST) */
	~SatelliteTerminalList();


	/**** operations ****/

	/* add a new Satellite Terminal (ST) in the list */
	bool add(long id,
	         unsigned long simu_column_num);

	/* delete a Satellite Terminal (ST) from the list */
	bool del(long id);

	/* does a ST with the given ID exist ? */
	bool do_exist(long id);

	/* clear the list of STs */
	void clear();

	/* go to next step in adaptive physical layer scenario */
	bool goNextScenarioStep();

	/* was the current MODCOD IDs of all the STs
	   advertised over the emulated network ? */
	bool areCurrentModcodsAdvertised();


	/**** accessors for the list ****/

	/* set simulation file for MODCOD */
	bool setModcodSimuFile(std::string filename);

	/* set simulation file for DRA scheme */
	bool setDraSchemeSimuFile(std::string filename);


	/**** accessors for one ST in the list ****/

	/* get the column # associated to the ST
	   whose ID is given as input */
	unsigned int getSimuColumnNum(long id);

	/* get the current MODCOD ID of the ST
	   whose ID is given as input */
	unsigned int getCurrentModcodId(long id);

	/* get the previous MODCOD ID of the ST
	   whose ID is given as input */
	unsigned int getPreviousModcodId(long id);

	/* was the current MODCOD ID of the ST whose ID is given as input
	   advertised over the emulated network ? */
	bool isCurrentModcodAdvertised(long id);

	/* get the current DRA scheme ID of the ST
	   whose ID is given as input */
	unsigned int getCurrentDraSchemeId(long id);

	/*  get the terminal ID corresponding to the lower modcod */
	long getTalIdCorrespondingToLowerModcod();

 private:

	/* update the current MODCOD IDs of all STs from MODCOD simulation file */
	bool goNextScenarioStepModcod();

	/* update the current DRA scheme IDs of all STs from DRA simulation file */
	bool goNextScenarioStepDraScheme();

	/* read the next line of the simulation file
	 * and store it in the appropriate list */
	bool setList(std::ifstream &simu_file,
	             vector<string> &list);

};

#endif
