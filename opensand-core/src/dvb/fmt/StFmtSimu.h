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

#include <stdint.h>

/**
 * @class StFmtSimu
 * @brief The internal representation of a Satellite Terminal (ST)
 */
class StFmtSimu
{
 private:

	/** The ID of the ST (called TAL ID or MAC ID elsewhere in the code) */
	long id;

	/** The current MODCOD ID of the ST */
	uint8_t current_modcod_id;

	/** The previous MODCOD ID of the ST (for down/forward) */
	uint8_t previous_modcod_id;

 public:

	/**** constructor/destructor ****/

	/**
	 * @brief Create
	 *
	 * @param id               the ID of the ST (called TAL ID or MAC ID elsewhere
	 *                         in the code)
	 * @param modcod_id        the initial MODCOD ID of the ST
	 */
	StFmtSimu(long id,
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
	long getId() const;

	/**
	 * @brief Get the column # associated to the ST for MODCOD simulation files
	 *
	 * @return  the column number for MODCOD simulation files
	 */
	unsigned long getSimuColumnNum() const;

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

	/**
	 * @brief Get the previous MODCOD ID of the ST (for down/forward)
	 *
	 * @return  the previous MODCOD ID of the ST
	 */
	uint8_t getPreviousModcodId() const;

};

#endif
