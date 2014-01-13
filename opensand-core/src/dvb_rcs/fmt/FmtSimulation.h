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
 * @brief The FMT simulation elements
 */
class FmtSimulation
{
 private:

	/** The internal map that stores all the STs */
	map<tal_id_t, StFmtSimu *> sts;

	/** The table of down/forward MODCOD definitions */
	FmtDefinitionTable fwd_modcod_def;

	/** The file stream for the down/forward MODCOD simulation file
	 *  Need pointer because ifstream is not copyable */
	ifstream *fwd_modcod_simu;

	/** The table of up/return MODCOD definitions */
	FmtDefinitionTable ret_modcod_def;

	/** The file stream for the up/return MODCOD simulation file
	 *  Need pointer because ifstream is not copyable */
	ifstream *ret_modcod_simu;

	/** Whether the MODCOD simulation file is defined or not */
	bool is_fwd_modcod_simu_defined;

	/** Whether the up/return MODCOD simulation file is defined or not */
	bool is_ret_modcod_simu_defined;

	/** A list of the current down/forward MODCOD */
	vector<string> fwd_modcod_list;

	/** A list of the current up/return MODCOD */
	vector<string> ret_modcod_list;

	/** A list of terminal to advertise for down/forward MODCOD */
	list<tal_id_t> need_advertise;

 public:

	/**** constructor/destructor ****/

	/* create a list of Satellite Terminals (ST) */
	FmtSimulation();

	/* destroy a list of Satellite Terminals (ST) */
	~FmtSimulation();


	/**
	 * @brief Add a new Satellite Terminal (ST) in the list
	 *
	 * @param id               the ID of the ST (called TAL ID or MAC ID elsewhere
	 *                         in the code)
	 * @param simu_column_num  the column # associated to the ST for MODCOD
	 *                         simulation files
	 * @return                 true if the addition is successful, false otherwise
	 */
	bool addTerminal(tal_id_t id,
	                 unsigned long simu_column_num);

	/**
	 * @brief Delete a Satellite Terminal (ST) from the list
	 *
	 * @param id  the ID of the ST
	 * @return    true if the deletion is successful, false otherwise
	 */
	bool delTerminal(tal_id_t id);

	/**
	 * @brief Does a ST with the given ID exist ?
	 *
	 * @param id  the ID we want to check for
	 * @return    true if a ST, false is it does not exist
	 */
	bool doTerminalExist(tal_id_t id) const;

	/**
	 * @brief Clear the list of STs
	 */
	void clear();

	/**
	 * @brief Go to next step in adaptive physical layer scenario
	 *
	 * Update current MODCODs IDs of all STs in the list.
	 */
	bool goNextScenarioStep();

	/**
	 * @brief Was the current down/forward MODCOD IDs of all the STs advertised
	 *        over the emulated network ?
	 *
	 * @return  true if the current MODCOD IDs of all the STs are already
	 *          advertised, false if they were not yet
	 */
	bool areCurrentFwdModcodsAdvertised();

	/**
	 * @brief Set definition file for down/forward MODCOD
	 *
	 * @param filename The MODCOD definition file
	 * @return true on success, false otherwise
	 */
	bool setForwardModcodDef(const string &filename);

	/**
	 * @brief Set simulation file for down/forward link MODCOD
	 *
	 * @param filename  the name of the file in which MODCOD scenario is described
	 * @return          true if the file exist and is valid, false otherwise
	 */
	bool setForwardModcodSimu(const string &filename);

	/**
	 * @brief Set definition file for up/return MODCOD
	 *
	 * @param filename The MODCOD definition file
	 * @return true on success, false otherwise
	 */
	bool setReturnModcodDef(const string &filename);

	/**
	 * @brief Set simulation file for up/return link MODCOD
	 *
	 * @param filename  the name of the file in which MODCOD scenario is described
	 * @return          true if the file exist and is valid, false otherwise
	 */
	bool setReturnModcodSimu(const string &filename);

	/**
	 * @brief Get the column # associated to the ST whose ID is given as input
	 *
	 * @param id  the ID of the ST
	 * @return    the column # associated to the ST
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	unsigned int getSimuColumnNum(tal_id_t id) const;

	/**
	 * @brief Get the current down/forward MODCOD ID of the ST whose ID is given as input
	 *
	 * @param id  the ID of the ST
	 * @return    the current MODCOD ID of the ST
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	unsigned int getCurrentFwdModcodId(tal_id_t id) const;

	/**
	 * @brief Get the previous MODCOD ID of the ST whose ID is given as input
	 *
	 * @param id  the ID of the ST
	 * @return    the previous MODCOD ID of the ST
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	unsigned int getPreviousFwdModcodId(tal_id_t id) const;

	/**
	 * @brief Get the higher forward MODCOD ID
	 *
	 * @return the highest forward MODCOD ID
	 */
	unsigned int getMaxFwdModcod() const;

	/**
	 * @brief Get the higher return MODCOD ID
	 *
	 * @return the highest return MODCOD ID
	 */
	unsigned int getMaxRetModcod() const;

	/**
	 * @brief Was the current down/forward MODCOD ID of the ST whose ID
	 *        is given as input  advertised over the emulated network ?
	 *
	 * @return  true if the MODCOD ID was already advertised,
	 *          false if it was not advertised yet
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	bool isCurrentFwdModcodAdvertised(tal_id_t id) const;

	/**
	 * Get the next terminal to advertise
	 *
	 * @param tal_id     OUT: The ID of the terminal to advertise
	 * @param modcod_id  OUT: The MODCOD ID for temrinal advertisement
	 * @return true if there ise a terminal to advertise, false otherwise
	 */
	bool getNextFwdModcodToAdvertise(tal_id_t &id, unsigned int &modcod_id);

	/**
	 * @brief Get the current up/return MODCOD ID of the ST whose ID is given as input
	 *
	 * @param id  the ID of the ST
	 * @return    the current up/return MODCOD ID of the ST
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	unsigned int getCurrentRetModcodId(tal_id_t id) const;

	/**
	 * @brief Get the terminal ID for wich the used down/forward
	 *        MODCOD is the lower
	 *
	 * @return terminal ID (should be positive, return -1 (255) if an error occurs)
	 */
	tal_id_t getTalIdWithLowerFwdModcod() const;

	/**
	 * @brief Get the MODCOD definitions for down/forward link
	 *
	 * @return the MODCOD definitions
	 */
	const FmtDefinitionTable *getFwdModcodDefinitions() const;

	/**
	 * @brief Get the MODCOD definitions for up/return link
	 *
	 * @return the MODCOD definitions
	 */
	const FmtDefinitionTable *getRetModcodDefinitions() const;

	/**
	 * @brief Set the required up/return MODCOD ID for of the
	 *        ST whid ID is given as input according to the required Es/N0
	 *
	 * @param id   the ID of the ST
	 * @param cni  the required Es/N0 for that terminal
	 */
	void setRetRequiredModcod(tal_id_t tal_id, double cni) const;

	/**
	 * @brief Set the required down/forward MODCOD ID for of the
	 *        ST whid ID is given as input according to the required Es/N0
	 *
	 * @param id   the ID of the ST
	 * @param cni  the required Es/N0 for that terminal
	 */
	void setFwdRequiredModcod(tal_id_t tal_id, double cni);

 private:

	/**
	 * @brief Update the current down/forwardMODCOD IDs of all STs
	 *        from MODCOD simulation file
	 *
	 * @return true on success, false on failure
	 */
	bool goNextScenarioStepFwdModcod();

	/**
	 * @brief Update the current up/return MODCOD IDs of all STs
	 *        from MODCOD simulation file
	 *
	 * @return true on success, false on failure
	 */
	bool goNextScenarioStepRetModcod();

	/**
	 * @brief Set the down/forward MODCOD advertised for terminal
	 *
	 * @param tal_id  The terminal ID
	 */
	void setFwdModcodAdvertised(tal_id_t tal_id);

	/**
	 * @brief Read a line of a simulation file and fill the MODCOD list
	 *
	 * @param   simu_file the simulation file (fwd_modcod_simu or ret_modcod_simu)
	 * @param   list      The MODCOD list
	 * @return            true on success, false on failure
	 *
	 * @todo better parsing
	 */
	bool setList(ifstream *simu_file,
	             vector<string> &list);

};

#endif
