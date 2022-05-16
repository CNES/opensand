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
 * @file    CarriersGroup.h
 * @brief   Represent a group of carriers with the same characteristics
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */

#ifndef _CARRIERS_GROUP_H_
#define _CARRIERS_GROUP_H_

#include "OpenSandCore.h"
#include "FmtGroup.h"

/**
 * @class CarriersGroup
 * @brief Represent a group of carriers with the same characteristics
 */
class CarriersGroup
{
 public:

	/**
	 * @brief  Construct a group of carriers with the same characteristics
	 *
	 * @param  carriers_id     The carriers ID
	 * @param  fmt_group       The FMT group
	 * @param  ratio           The estimated occupation ratio
	 * @param  rate_syms       The symbol rate (sym/s)
	 * @param  access_type     The carriers access type
	 */
	CarriersGroup(unsigned int carriers_id,
	              const FmtGroup *const fmt_group,
	              unsigned int ratio,
	              rate_symps_t rate_symps,
	              AccessType access_type);

	/** Destructor */
	virtual ~CarriersGroup();

	/**
	 * @brief   Get carriers Id.
	 *
	 * @return  carriers Id.
	 */
	unsigned int getCarriersId() const;

	/**
	 * @brief Set the number of carriers with this characteristics
	 *
	 * @param carriers_number  The number of carriers
	 */
	virtual void setCarriersNumber(const unsigned int carriers_number);

	/**
	 * @brief  Set carriers capacity.
	 *
	 * @param  capacity_sym  carriers capacity (in symbols) for a superframe.
	 */
	virtual void setCapacity(const vol_sym_t capacity_sym);

	/**
	 * @brief   Get carriers group capacity
	 *
	 * @return  carriers group capacity (in symbols) for a superframe.
	 */
	vol_sym_t getTotalCapacity() const;

	/**
	 * @brief  Get carriers's symbol rate.
	 *
	 * @return  symbol rate of carriers.
	 */
	rate_symps_t getSymbolRate() const;

	/**
	 * @brief  Set carriers's symbol rate.
	 *
	 * @param  symbol_rate  new symbol rate.
	 */
	virtual void setSymbolRate(const rate_symps_t symbol_rate_symps);

	/**
	 * @brief  Get the carriers number
	 *
	 * @return  the carriers number
	 */
	unsigned int getCarriersNumber() const;

	/**
	 * @brief  Get the estimated occupation ratio
	 *
	 * @return  The estimated occupetion ratio
	 */
	unsigned int getRatio() const;

 	/**
	 * @brief   Set the estimated occupation ratio
	 *
	 * @param  The new estimated occupation ratio
	 */
	void setRatio(unsigned int new_ratio);

	/**
	 * @brief Get the list of available MODCODs in carrier
	 *
	 * @return the list of MODCODs
	 */
	const list<fmt_id_t> getFmtIds() const;

	/**
	 * @brief Get the carriers access type
	 *
	 * @return  The carriers access type
	 */
	AccessType getAccessType(void) const;

 	/**
	 * @brief Get the FMT Group
	 *
	 * @return The FMT Group
	 */
	const FmtGroup *getFmtGroup() const;

	/**
	 * @brief  Get the maximum rate available with this carriers
	 *
	 * @return The rate available with this carriers
	 */
	rate_kbps_t getMaxRate() const;

	/**
	 * @brief Add a VCM part in the carriers group
	 *
	 * @param  fmt_group       The FMT group
	 * @param  ratio           The estimated occupation ratio
	 */
	virtual void addVcm(const FmtGroup *const fmt_group,
	                    unsigned int ratio);

 protected:

	/** Carriers id */
	unsigned int carriers_id;

	/** FMT group */
	const FmtGroup *fmt_group;

	/** The number of carriers with this characteristics */
	unsigned int carriers_number;

	/** The estimated occupation ratio */
	unsigned int ratio;

	/** The total capacity of each carriers (symbol number) */
	vol_sym_t capacity_sym;

	/** Symbol rate (symbol per second) */
	rate_symps_t symbol_rate_symps;

	/** Access type */
	AccessType access_type;
};


#endif


