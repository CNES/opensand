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


/**
 *  @class DamaCtrlRcsLegacy
 *  @brief This library defines the legacy DAMA controller.
 */
class DamaCtrlRcsLegacy: public DamaCtrlRcs
{

 public:

	DamaCtrlRcsLegacy();
	virtual ~DamaCtrlRcsLegacy();


 protected:
	/// output probes
	static Probe<int> *probe_gw_fca_alloc;
	static Probe<float> *probe_gw_uplink_fair_share;

 private:
	///RBDC allocation
	bool runDamaRbdc();
	/// VBDC allocation
	bool runDamaVbdc();
	/// FCA allocation
	bool runDamaFca();
	/// reset DAMA
	bool resetDama();
	/// update statistics
	void updateStatistics();

	virtual bool init();

	/**
	 * @brief Compute RBDC per carriers group
	 *
	 * @param carriers  The carrier group
	 * @param category  The terminal category containing the carrier
	 */
	void runDamaRbdcPerCarrier(CarriersGroup *carriers,
	                           const TerminalCategory *category);

	/**
	 * @brief Compute VBDC per carriers group
	 *
	 * @param carriers  The carrier group
	 * @param category  The terminal category containing the carrier
	 */
	void runDamaVbdcPerCarrier(CarriersGroup *carriers,
	                           const TerminalCategory *category);

	/**
	 * @brief Compute FCA per carriers group
	 *
	 * @param carriers  The carrier group
	 * @param category  The terminal category containing the carrier
	 */
	void runDamaFcaPerCarrier(CarriersGroup *carriers,
	                          const TerminalCategory *category);

};

#endif
