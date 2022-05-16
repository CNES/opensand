/*
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
 * @file    CarriersGroupDama.h
 * @brief   Represent a group of carriers with the same characteristics
 *          for DAMA
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */

#ifndef _CARRIERS_GROUP_DAMA_H_
#define _CARRIERS_GROUP_DAMA_H_

#include "CarriersGroup.h"


/**
 * @class CarriersGroupDama
 * @brief Represent a group of carriers with the same characteristics
 *        for DAMA
 */
class CarriersGroupDama: public CarriersGroup
{
 public:
	/**
	 * @brief  Construct a group of carriers with the same characteristics
	 *
	 * @param  carriers_id        The carriers ID
	 * @param  fmt_group          The FMT group
	 * @param  ratio              The estimated occupation ratio
	 * @param  symbol_rate_symps  The symbol rate (sym/s)
	 * @param  access_type        The carriers access type
	 */
	CarriersGroupDama(unsigned int carriers_id,
	                  const FmtGroup *const fmt_group,
	                  unsigned int ratio,
	                  rate_symps_t symbol_rate_symps,
	                  AccessType access_type);

	/** Destructor */
	virtual ~CarriersGroupDama();


	virtual void setCapacity(const vol_sym_t capacity_sym);
	virtual void setCarriersNumber(const unsigned int carriers_number);
	virtual void setSymbolRate(const rate_symps_t symbol_rate_symps);
	virtual void addVcm(const FmtGroup *const fmt_group,
	                    unsigned int ratio);

	/**
	 * @brief  Get available capacity.
	 * @warning  As this value is only used locally in DAMA Controller,
	 *           the unit can be choosen according to the most conveninent
	 *           one.
	 *
	 * @return  available capacity
	 */
	unsigned int getRemainingCapacity() const;

	/**
	 * @brief  Set available capacity.
	 * @warning  The remaining capacity should be reseted with total capacity
	 *           before DAMA computation (in @ref resetDama function usually)
	 *           As it is only used locally in DAMA Controller, the unit can
	 *           be choosen according to the most conveninent one.
	 *
	 * @param remaining_capacity  available capacity
	 */
	void setRemainingCapacity(const unsigned int remaining_capacity);

	/**
	 * @brief  Get previous capacity.
	 *         This can be used if all the capacity has not been used and can
	 *         be consumed on next frames
	 * @warning  As this value is only used locally in DAMA Controller,
	 *           the unit can be choosen according to the most conveninent
	 *           one.
	 *
	 * @param superframe_sf  The current superframe
	 * @param frame          The current frame
	 * @return  available capacity if this is actually the next frame
	 */
	unsigned int getPreviousCapacity(const time_sf_t superframe_sf) const;

	/**
	 * @brief  Set available capacity.
	 *
	 * @param superframe_sf  The next superframe
	 * @param previous_capacity  available capacity for next frame
	 */
	void setPreviousCapacity(const unsigned int previous_capacity,
	                         const time_sf_t superframe_sf);

	/**
	 * @brief Get the nearest supported value in the FMT group
	 *
	 * @param fmt_id  The desired FMT id
	 * @return The nearest available FMT id if found
	 *         0 if there is no supported FMT id
	 */
	unsigned int getNearestFmtId(unsigned int fmt_id);

	/**
	 * @brief Get the VCM carriers
	 *
	 * @return the VCM carriers
	 */
	std::vector<CarriersGroupDama *> getVcmCarriers();

 protected:
	/** The remaining capacity on the current frame */
	unsigned int remaining_capacity;

	/** The previous capacity */
	unsigned int previous_capacity;

	/** The superframe for which we can get the previous capacity */
	time_sf_t previous_sf;

	/** In case of VCM, this carriers group contains only global values over
	 *  the entire frame (total ratio, total capacity, ...) and each VCM part
	 *  is instantiated into a new carriers group */
	std::vector<CarriersGroupDama *> vcm_carriers;
};

#endif


