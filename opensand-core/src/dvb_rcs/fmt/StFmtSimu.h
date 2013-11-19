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
 * @file StFmtSimu.h
 * @brief The satellite temrinal simulated FMT values
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien Bernad / Viveris Technologies
 */

#ifndef ST_FMT_SIMU_H
#define ST_FMT_SIMU_H


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

	/** The current down/forward MODCOD ID of the ST */
	unsigned int current_fwd_modcod_id;

	/** The previous down/forward MODCOD ID of the ST */
	unsigned int previous_fwd_modcod_id;

	/**
	 * Whether the current down/forward MODCOD ID was advertised to the ST
	 * over the emulated satellite network
	 */
	bool is_current_modcod_advertised;

	/** The current up/return MODCOD ID of the ST */
	unsigned int current_ret_modcod_id;

 public:

	/**** constructor/destructor ****/

	/**
	 * @brief Create
	 *
	 * @param id               the ID of the ST (called TAL ID or MAC ID elsewhere
	 *                         in the code)
	 * @param simu_column_num  the column # associated to the ST for MODCOD
	 *                         simulation files
	 * @param fwd_modcod_id    the initial down/forward MODCOD ID of the ST
	 * @param ret_modcod_id    the initial up/return MODCOD ID of the ST
	 */
	StFmtSimu(long id,
	          unsigned long simu_column_num,
	          unsigned int fwd_modcod_id,
	          unsigned int ret_modcod_id);

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
	 * @brief Get the current down/forward MODCOD ID of the ST
	 *
	 * @return  the current down/forward MODCOD ID of the ST
	 */
	unsigned int getCurrentFwdModcodId() const;

	/**
	 * @brief Update the down/forward MODCOD ID of the ST
	 *
	 * @param new_id     the new down/forward MODCOD ID of the ST
	 * @param advertise  whether we should set advertise if the MODCOD changed
	 */
	void updateFwdModcodId(unsigned int new_id, bool advertise=true);

	/**
	 * @brief Get the previous down/forward MODCOD ID of the ST
	 *
	 * @return  the previous down/forward MODCOD ID of the ST
	 */
	unsigned int getPreviousFwdModcodId() const;

	/**
	 * @brief Was the current down/forward MODCOD ID advertised to the ST ?
	 *
	 * @return  true if the current down/forward MODCOD ID was already advertised to the ST,
	 *          false if it was not advertised yet
	 */
	bool isCurrentFwdModcodAdvertised() const;

	/**
	 * @brief Set the down/forward MODCOD ID avertised for the ST
	 */
	void setFwdModcodAdvertised(void);

	/**
	 * @brief Get the current up/return MODCOD ID of the ST
	 *
	 * @return  the current up/return MODCOD ID of the ST
	 */
	unsigned int getCurrentRetModcodId() const;

	/**
	 * @brief Update the up/return MODCOD ID of the ST
	 *
	 * @param new_id  the new up/return MODCOD ID of the ST
	 */
	void updateRetModcodId(unsigned int new_id);

};

#endif
