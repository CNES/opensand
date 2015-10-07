/*
 *
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
 * @file StFmtSimu.h
 * @brief The satellite temrinal simulated FMT values
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien Bernad / Viveris Technologies
 */

#ifndef ST_FMT_SIMU_H
#define ST_FMT_SIMU_H

#include <OpenSandCore.h>
#include <FmtSimulation.h>

#include <opensand_rt/RtMutex.h>
#include <opensand_output/Output.h>

#include <stdint.h>

#include <map>
#include <set>

/**
 * @class StFmtSimu
 * @brief The internal representation of a Satellite Terminal (ST)
 */
class StFmtSimu
{
 private:

	/** The ID of the ST (called TAL ID or MAC ID elsewhere in the code) */
	tal_id_t id;
	
	/** The column used to read FMT id */
	unsigned long column;

	/** The current MODCOD ID of the ST */
	uint8_t current_modcod_id;

	mutable RtMutex modcod_mutex; ///< The mutex to protect the modcod from concurrent access


 public:

	/**** constructor/destructor ****/

	/**
	 * @brief Create
	 *
	 * @param id               the ID of the ST (called TAL ID or MAC ID elsewhere
	 *                         in the code)
	 * @param modcod_id        the initial MODCOD ID of the ST
	 */
	StFmtSimu(tal_id_t id,
	          uint8_t modcod_id);

	/* destroy an internal representation of a Satellite Terminal (ST) */
	~StFmtSimu();


	/**** accessors ****/

	/**
	 * @brief Get the ID of the ST
	 *
	 * The ID of the ST is often called TAL ID or MAC ID elsewhere in the code
	 *
	 * @return  the ID of the ST
	 */
	tal_id_t getId() const;

	/**
	 * @brief Get the column # associated to the ST for MODCOD simulation files
	 *
	 * @return  the column number for MODCOD simulation files
	 */
	unsigned long getSimuColumnNum() const;


	/**
	 * @brief Set the column # associated to the ST for MODCOD simulation files
	 *
	 * @param   the column number for MODCOD simulation files
	 */
	void setSimuColumnNum(unsigned long col);

	/**
	 * @brief Get the current MODCOD ID of the ST
	 *
	 * @return  the current  MODCOD ID of the ST
	 */
	uint8_t getCurrentModcodId();

	/**
	 * @brief Update the MODCOD ID of the ST
	 *
	 * @param new_id     the new MODCOD ID of the ST
	 */
	void updateModcodId(uint8_t new_id);

};


typedef std::map<tal_id_t, StFmtSimu *> ListStFmt;


/**
 * @class StFmtSimuList
 * @brief The List of StFmtSimu per spot
 */
class StFmtSimuList
{
 private:

	/** the list of StFmtSimu per spot */
	ListStFmt *sts;

	// Output Log
	OutputLog *log_fmt;

	/** a list which associate a st id with its spot id */
	/** the mutex to protect the list from concurrent access */
	mutable RtMutex sts_mutex;


 public:

	/// Constructor and destructor
	StFmtSimuList();
	~StFmtSimuList();


	/**
	 * @brief   get the list
	 *
	 * @return  the list
	 */
	const ListStFmt *getListSts(void) const;

	/**
	 * @brief  add a terminal in the list
	 *
	 * @param  st_id    the id of the terminal
	 * @param  modcod   the initial modcod of the terminal
	 * @return  true on succes, false otherwise
	 */
	bool addTerminal(tal_id_t st_id, fmt_id_t modcod);

	/**
	 * @brief  remove a terminal from the list
	 *
	 * @param  st_id    the id of the terminal
	 * @return  true on succes, false otherwise
	 */
	bool delTerminal(tal_id_t st_id);

	/**
	 * @brief  update the modcod of all st in the list according
	 *         to the simulation file
	 *
	 * @param fmt_simu  the fmt simulation
	 */
	void updateModcod(const FmtSimulation &fmt_simu);

	/**
	 * @brief  set the modcod id for the st
	 *
	 * @param  st_id      the id of the st
	 * @param  modcod_id  the id of the modcod
	 */
	void setRequiredModcod(tal_id_t st_id, fmt_id_t modcod_id);

	/**
	 * @brief  get the current modcod id for a st
	 *
	 * @param  st_id  the id of the st
	 * @return the modcod id
	 */
	uint8_t getCurrentModcodId(tal_id_t st_id) const;

	/**
	 * @brief  check is the st is present
	 *
	 * @param  st_id   the id of the st
	 * @return true is present, false otherwise
	 */
	bool isStPresent(tal_id_t st_id) const;

};

#endif
