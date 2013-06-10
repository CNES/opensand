/*
 *
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
 * @file SatelliteTerminal.h
 * @brief The internal representation of a Satellite Terminal (ST)
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef SATELLITE_TERMINAL_H
#define SATELLITE_TERMINAL_H


/**
 * @class SatelliteTerminal
 * @brief The internal representation of a Satellite Terminal (ST)
 */
class SatelliteTerminal
{
 private:

	/** The ID of the ST (called TAL ID or MAC ID elsewhere in the code) */
	long id;

	/** The column # associated to the ST for DRA/MODCOD simulation files */
	unsigned long simu_column_num;

	/** The current MODCOD ID of the ST */
	unsigned int current_modcod_id;

	/** The previous MODCOD ID of the ST */
	unsigned int previous_modcod_id;

	/**
	 * Whether the current MODCOD ID was advertised to the ST
	 * over the emulated satellite network
	 */
	bool is_current_modcod_advertised;

	/** The current DRA scheme ID of the ST */
	unsigned int current_dra_scheme_id;

 public:

	/**** constructor/destructor ****/

	/* create an internal representation of a Satellite Terminal (ST) */
	SatelliteTerminal(long id,
	                  unsigned long simu_column_num,
	                  unsigned int modcod_id,
	                  unsigned int dra_scheme_id);

	/* destroy an internal representation of a Satellite Terminal (ST) */
	~SatelliteTerminal();


	/**** accessors ****/

	/* get the ID of the ST */
	long getId();

	/* get the column # associated to the ST for DRA/MODCOD simulation files */
	unsigned long getSimuColumnNum();

	/* get the current MODCOD ID of the ST */
	unsigned int getCurrentModcodId();

	/* update the MODCOD ID of the ST */
	void updateModcodId(unsigned int new_id);

	/* get the previous MODCOD ID of the ST */
	unsigned int getPreviousModcodId();

	/* Was the current MODCOD ID advertised to the ST ? */
	bool isCurrentModcodAdvertised();

	/* get the current DRA scheme ID of the ST */
	unsigned int getCurrentDraSchemeId();

	/* update the DRA scheme ID of the ST */
	void updateDraSchemeId(unsigned int new_id);

};

#endif
