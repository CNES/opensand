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
	uint8_t getCurrentModcodId() const;

	/**
	 * @brief Update the MODCOD ID of the ST
	 *
	 * @param new_id     the new MODCOD ID of the ST
	 */
	void updateModcodId(uint8_t new_id);

};



typedef std::map<tal_id_t, StFmtSimu *> ListSts;
typedef std::map<spot_id_t, ListSts *> ListStsPerSpot;
typedef std::map<tal_id_t, ListStsPerSpot *> ListStsPerSpotPerGw;


/**
 * @class StFmtSimuList
 * @brief The List of StFmtSimu
 */
class StFmtSimuList
{
 private:

	/** the list of StFmtSimu per spot and per gw */
	ListStsPerSpotPerGw sts;

	// Output Log
	OutputLog *log_fmt;

	/** a list which associate a st id with it gw id and spot id */
	std::map<tal_id_t, std::pair<spot_id_t, tal_id_t> > sts_ids;

	/** a list of gw id */
	std::set<tal_id_t> gws_id;

	/** the mutex to protect the list from concurrent access */
	mutable RtMutex sts_mutex;

	/**
	 * @brief  get the ListStsPerSpot for the gw asked
	 * @warning  this function desn't lock the mutex, only use it in
	 *           a function that lock the mutex
	 *
	 * @param  gw_id   the tal id of the gw
	 * @return the ListStsPerSpot for the gw asked
	 */
	ListStsPerSpot* getListStsPerSpot(tal_id_t gw_id) const;

	/**
	 * @brief   get the ListSts for the right spot and gw
	 * @warning  this function desn't lock the mutex, only use it in
	 *           a function that lock the mutex
	 *
	 * @param   gw_id the    tal id of the gw
	 * @param   spot_id      the id of the spot
	 * @return  the ListSts for the right spot and gw
	 */
	ListSts* getListStsPriv(tal_id_t gw_id, spot_id_t spot_id) const;

	/**
	 * @brief  set the ListSts for the right gw and spot
	 * @warning  this function desn't lock the mutex, only use it in
	 *           a function that lock the mutex
	 *
	 * @param   gw_id the    tal id of the gw
	 * @param   spot_id      the id of the spot
	 * @param   list_sts     the list to set
	 * @return  true on success, false otherwise
	 */
	bool setListStsPriv(tal_id_t gw_id, spot_id_t spot_id, ListSts* list_sts);

	/**
	 * @brief  set the modcod id for the gw
	 * @warning  this function desn't lock the mutex, only use it in
	 *           a function that lock the mutex
	 *
	 * @param  gw_id      the id of the gw
	 * @param  modcod_id  the id of the modcod
	 */
	void setRequiredModcodGw(tal_id_t gw_id, uint8_t modcod_id);

	/**
	 * @brief  get the current modcod id for a gw
	 * @warning  this function desn't lock the mutex, only use it in
	 *           a function that lock the mutex
	 *
	 * @param  gw_id  the id of the gw
	 * @return the modcod id
	 */
	uint8_t getCurrentModcodIdGw(tal_id_t gw_id);

	void clearListStsPerSpot(ListStsPerSpot* list);

	void clearListSts(ListSts* list);


 public:

	/// Constructor and destructor
	StFmtSimuList();
	~StFmtSimuList();

	/**
	 * @brief  The getter of sts
	 *
	 * @return sts
	 */
	ListStsPerSpotPerGw getSts(void) const;

	/**
	 * @brief  set the ListStsPerSpot for the gw asked
	 *
	 * @param  gw_id               the tal id of the gw
	 * @param  list_sts_per_spot   the list to set
	 * @return true on success, false otherwise
	 */
	bool setListStsPerSpot(tal_id_t gw_id, ListStsPerSpot* list_sts_per_spot);

	/**
	 * @brief   get the ListSts for the right spot and gw
	 *
	 * @param   gw_id the    tal id of the gw
	 * @param   spot_id      the id of the spot
	 * @return  the ListSts for the right spot and gw
	 */
	ListSts* getListSts(tal_id_t gw_id, spot_id_t spot_id) const;

	/**
	 * @brief  set the ListSts for the right gw and spot
	 *
	 * @param   gw_id the    tal id of the gw
	 * @param   spot_id      the id of the spot
	 * @param   list_sts     the list to set
	 * @return  true on success, false otherwise
	 */
	bool setListSts(tal_id_t gw_id, spot_id_t spot_id, ListSts* list_sts);

	/**
	 * @brief  add a terminal in the right list
	 *
	 * @param  st_id    the id of the terminal
	 * @param  modcod   the modcod of the terminal
	 * @param  gw_id    the id of the gw associated
	 * @param  spot_id  the id of the spot associated
	 * @return  true on succes, false otherwise
	 */
	bool addTerminal(tal_id_t st_id, uint8_t modcod, tal_id_t gw_id, spot_id_t spot_id);

	/**
	 * @brief  add a terminal in the right list
	 *
	 * @param  st_id    the id of the terminal
	 * @param gw_id     the id of the gw
	 * @param spot_id   the id of the spot
	 * @return  true on succes, false otherwise
	 */
	bool delTerminal(tal_id_t st_id, tal_id_t gw_id, spot_id_t spot_id);

	/**
	 * @brief  update the modcod of all st in the list
	 *         for a gw and a spot
	 *
	 * @param gw_id     the id of the gw
	 * @param spot_id   the id of the spot
	 * @param fmt_simu  the fmt simulation
	 */
	void updateModcod(tal_id_t gw_id, spot_id_t spot_id, FmtSimulation* fmt_simu);

	/**
	 * @brief  set the modcod id for the st
	 *         If st_id is the id of a gw, we must update
	 *         the modcod for all the spot
	 *
	 * @param  st_id      the id of the st
	 * @param  modcod_id  the id of the modcod
	 */
	void setRequiredModcod(tal_id_t st_id, uint8_t modcod_id);

	/**
	 * @brief  get the current modcod id for a st
	 *
	 * @param  st_id  the id of the st
	 * @return the modcod id
	 */
	uint8_t getCurrentModcodId(tal_id_t st_id);

	/**
	 * @brief  check is the st is present
	 *
	 * @param  st_id   the id of the st
	 * @return true is present, false otherwise
	 */
	bool isStPresent(tal_id_t st_id);

};

#endif
