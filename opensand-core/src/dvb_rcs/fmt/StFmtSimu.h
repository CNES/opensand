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

	/** The column # associated to the ST for down/forward MODCOD simulation files */
	unsigned long simu_column_num;

	/** The current MODCOD ID of the ST */
	uint8_t current_modcod_id;

	/** The previous MODCOD ID of the ST (for down/forward) */
	uint8_t previous_modcod_id;

	/**
	 * Whether the current MODCOD ID was advertised to the ST
	 * over the emulated satellite network (for down/forward)
	 */
	bool is_current_modcod_advertised;

 public:

	/**** constructor/destructor ****/

	/**
	 * @brief Create
	 *
	 * @param id               the ID of the ST (called TAL ID or MAC ID elsewhere
	 *                         in the code)
	 * @param simu_column_num  the column # associated to the ST for MODCOD
	 *                         simulation files
	 * @param modcod_id        the initial MODCOD ID of the ST
	 */
	StFmtSimu(long id,
	          unsigned long simu_column_num,
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
	 * @param advertise  whether we should set advertise if the MODCOD changed
	 *                   (for down/forward)
	 */
	void updateModcodId(uint8_t new_id, bool advertise=true);
	// TODO advertise false by default

	/**
	 * @brief Get the previous MODCOD ID of the ST (for down/forward)
	 *
	 * @return  the previous MODCOD ID of the ST
	 */
	uint8_t getPreviousModcodId() const;

	/**
	 * @brief Was the current MODCOD ID advertised to the ST (for down/forward) ?
	 *
	 * @return  true if the current MODCOD ID was already advertised to the ST,
	 *          false if it was not advertised yet
	 */
	bool isCurrentModcodAdvertised() const;

	/**
	 * @brief Set the MODCOD ID avertised for the ST (for down/forward) 
	 */
	void setModcodAdvertised(void);

};

#endif
