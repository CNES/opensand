/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Adrien Thibaud <athibaud@toulouse.viveris.com>
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 */

#ifndef ST_FMT_SIMU_H
#define ST_FMT_SIMU_H

#include <OpenSandCore.h>
#include <FmtDefinitionTable.h>

#include <opensand_rt/RtMutex.h>
#include <opensand_output/Output.h>

#include <stdint.h>

#include <map>
#include <set>


using std::set;

/**
 * @class StFmtSimu
 * @brief The internal representation of a Satellite Terminal (ST)
 */
class StFmtSimu
{
	friend class StFmtSimuList;
 private:

	/** The ID of the ST (called TAL ID or MAC ID elsewhere in the code) */
	tal_id_t id;

	/** The MODCOD definitions for the terminal and associated link */
	const FmtDefinitionTable *const modcod_def;
	
	/** the cni status*/
	bool cni_has_changed;
	
	/** The column used to read FMT id */
	unsigned long column;

	/** The current MODCOD ID of the ST */
	uint8_t current_modcod_id;

	// Output Log
	OutputLog *log_fmt;

	/// The functions are private because they are not protected by a mutex as
	//  they are used internally or by StFmtSimuList which is protected by a mutex and
	//  is therefore a friend class

	/**** constructor/destructor ****/

	/**
	 * @brief Create
	 *
	 * @param name             a name to know if this is input or output terminals
	 * @param id               the ID of the ST (called TAL ID or MAC ID elsewhere
	 *                         in the code)
	 * @param modcod_id        the initial MODCOD ID of the ST
	 * @param modcod_def       the MODCOD definition for the terminal and associated link
	 */
	StFmtSimu(string name,
	          tal_id_t id,
	          uint8_t init_modcod_id,
	          const FmtDefinitionTable *const modcod_def);

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
	 * @param new_id              the new MODCOD ID of the ST
	 * @param acm_loop_margin_db  The ACM loop margin
	 */
	void updateModcodId(uint8_t new_id, double acm_loop_margin_db=0.0);

	/**
	 * @brief Update the MODCOD ID of the ST with a CNI value
	 *
	 * @param cni                 The new CNI
	 * @param acm_loop_margin_db  The ACM loop margin
	 */
	void updateCni(double cni, double acm_loop_margin_db);




	/**
	 * @brief get the required CNI value depending on current MODCOD ID
	 *
	 * @return the current CNI value or 0.0 on error
	 */
	double getRequiredCni();

	/**
	 * @brief get the cni change status
	 *
	 * @return the cni status
	 */
	bool getCniHasChanged();

};




/**
 * @class StFmtSimuList
 *        The class is also a list of registered terminal IDs
 * @brief The List of StFmtSimu per spot
 */
class StFmtSimuList: public set<tal_id_t>
{
 private:
	typedef std::map<tal_id_t, StFmtSimu *> ListStFmt;

	/** A name to know is this is input or output terminals */
	string name;

	/** the list of StFmtSimu per spot */
	ListStFmt *sts;

	/** The ACM loop margin */
	double acm_loop_margin_db;

	// Output Log
	OutputLog *log_fmt;

	/** a list which associate a st id with its spot id */
	/** the mutex to protect the list from concurrent access */
	mutable RtMutex sts_mutex;


 public:

	/// Constructor and destructor
	StFmtSimuList(string name);
	~StFmtSimuList();


	/**
	 * @brief  Set the ACM loop margin value
	 *
	 * @param acm_loop_margin_db  The ACM loop margin
	 */
	void setAcmLoopMargin(double acm_loop_margin_db);

	/**
	 * @brief  add a terminal in the list
	 *
	 * @param  st_id         the id of the terminal
	 * @param  init_modcod   the initial modcod of the terminal
	 * @param  modcod_def    the MODCOD definitions for the terminal
	 * @return  true on succes, false otherwise
	 */
	bool addTerminal(tal_id_t st_id, fmt_id_t init_modcod,
	                 const FmtDefinitionTable *const modcod_def);

	/**
	 * @brief  remove a terminal from the list
	 *
	 * @param  st_id    the id of the terminal
	 * @return  true on succes, false otherwise
	 */
	bool delTerminal(tal_id_t st_id);

	/**
	 * @brief Set the CNI of a terminal
	 *
	 * @param st_id  The terminal ID
	 * @param cni    The CNI value
	 */
	void setRequiredCni(tal_id_t st_id, double cni);

	/**
	 * @brief Get the required CNI of a terminal
	 *
	 * @param st_id  The terminal ID
	 * @return the required CNI of the terminal or 0.0 on error
	 */
	double getRequiredCni(tal_id_t st_id) const;


	/**
	 * @brief  get the current modcod id for a st
	 *
	 * @param  st_id  the id of the st
	 * @return the modcod id
	 */
	uint8_t getCurrentModcodId(tal_id_t st_id) const;
	
	/**
	 * @brief get the cni change status
	 *
	 * @param  st_id  the id of the st
	 */
	bool getCniHasChanged(tal_id_t st_id);

	/**
	 * @brief  check is the st is present
	 *
	 * @param  st_id   the id of the st
	 * @return true is present, false otherwise
	 */
	bool isStPresent(tal_id_t st_id) const;

	/**
	 * @brief  get the terminal ID with the lowest MODCOD id in the list
	 *
	 * @return the terminal ID with the lowest MODCOD id
	 */
	tal_id_t getTalIdWithLowerModcod() const;
};

#endif
