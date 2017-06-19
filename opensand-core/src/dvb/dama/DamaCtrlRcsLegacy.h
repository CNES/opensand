/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file DamaCtrlRcsLegacy.h
 * @brief This library defines the legacy DAMA controller.
 *
 * @author ASP - IUSO, DTP (B. BAUDOIN)
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef _DAMA_CONTROLLER_RCS_LEGACY_H
#define _DAMA_CONTROLLER_RCS_LEGACY_H

#include "DamaCtrlRcs.h"

#include "OpenSandCore.h"
#include "CarriersGroup.h"
#include "TerminalCategoryDama.h"

/**
 *  @class DamaCtrlRcsLegacy
 *  @brief This library defines the legacy DAMA controller.
 */
class DamaCtrlRcsLegacy: public DamaCtrlRcs
{

 public:

	DamaCtrlRcsLegacy(spot_id_t spot, vol_b_t packet_length_b);
	virtual ~DamaCtrlRcsLegacy();

 private:

	/// initialize
	virtual bool init();

	 ///  Reset the capacity of carriers
	virtual bool resetCarriersCapacity();

	/// RBDC allocation
	virtual bool computeDamaRbdc();

	/// VBDC allocation
	virtual bool computeDamaVbdc();

	/// FCA allocation
	virtual bool computeDamaFca();

	/**
	 * @brief Compute RBDC per carriers group
	 *
	 * @param carriers           The carrier group
	 * @param category           The terminal category containing the carrier
	 * @param request_rate_kbps  The requested rate in kbit/s
	 * @param alloc_rate_kbps    The allocated rate in kbit/s
	 */
	void computeDamaRbdcPerCarrier(CarriersGroupDama *carriers,
	                               const TerminalCategoryDama *category,
	                               rate_kbps_t &request_rate_kbps,
	                               rate_kbps_t &alloc_rate_kbps);

	/**
	 * @brief Compute VBDC per carriers group
	 *
	 * @param carriers        The carrier group
	 * @param category        The terminal category containing the carrier
	 * @param request_vol_kb  The requested volume in kbit
	 * @param alloc_vol_kb    The allocated volume in kbit
	 */
	void computeDamaVbdcPerCarrier(CarriersGroupDama *carriers,
	                               const TerminalCategoryDama *category,
	                               vol_kb_t &request_vol_kb,
	                               vol_kb_t &alloc_vol_kb);

	/**
	 * @brief Compute FCA per carriers group
	 *
	 * @param carriers           The carrier group
	 * @param category           The terminal category containing the carrier
	 * @param alloc_rate_kbps    The allocated rate in kbit/s
	 */
	void computeDamaFcaPerCarrier(CarriersGroupDama *carriers,
	                              const TerminalCategoryDama *category,
	                              rate_kbps_t &alloc_rate_kbps);

};

#endif
