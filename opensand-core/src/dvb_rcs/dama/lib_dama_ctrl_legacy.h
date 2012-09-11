/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 * @file lib_dama_ctrl_legacy.h
 * @brief This library defines the legacy DAMA controller.
 *
 * @author ASP - IUSO, DTP (B. BAUDOIN)
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef LIB_DAMA_CTRL_Legacy_H
#define LIB_DAMA_CTRL_Legacy_H

#include "lib_dama_ctrl.h"
#include "lib_dama_utils.h"


/**
 *  @class DvbRcsDamaCtrlLegacy
 *  @brief This library defines the legacy DAMA controller.
 */
class DvbRcsDamaCtrlLegacy: public DvbRcsDamaCtrl
{

 public:

	DvbRcsDamaCtrlLegacy();
	virtual ~ DvbRcsDamaCtrlLegacy();

 protected:
	/// Environment plane probes
	static Probe<int>* probe_gw_fca_alloc;
	static Probe<float>* probe_gw_uplink_fair_share;

 private:
	/// the core of the class
	int runDama();

	///RBDC allocation
	int runDamaRbdc(int);
	/// VBDC allocation
	int runDamaVbdc(int);
	/// FCA allocation
	int runDamaFca(int);

	/// in charge of the round robin management
	DC_St *RoundRobin(int *);

};

#endif
