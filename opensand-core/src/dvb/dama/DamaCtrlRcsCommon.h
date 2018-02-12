/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
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

/*
 * @file DamaCtrlRcsCommon.h
 * @brief This library defines DAMA controller interfaces.
 * @author satip6 (Eddy Fromentin)
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef _DAMA_CONTROLLER_RCS_COMMON_H_
#define _DAMA_CONTROLLER_RCS_COMMON_H_

#include "DamaCtrl.h"
#include "TerminalContextDamaRcs.h"
#include "UnitConverter.h"

#include <opensand_output/Output.h>

/**
 * @class DamaCtrlRcsCommon
 * @brief Define methods to process DAMA request in the NCC
 */
class DamaCtrlRcsCommon: public DamaCtrl
{
 public:

	DamaCtrlRcsCommon(spot_id_t spot);
	virtual ~DamaCtrlRcsCommon();

	/**
	 * @brief  Initializes internal data structure according to configuration file
	 *
	 * @return  true on success, false otherwise
	 */
	virtual bool init();

	// Process DVB frames
	virtual bool hereIsSAC(const Sac *sac);

	// Build allocation table
	virtual bool buildTTP(Ttp *ttp);

	// Apply a PEP command
	virtual bool applyPepCommand(const PepRequest* request);
	
	// Update the required FMTs
	virtual void updateRequiredFmts();

 protected:

	UnitConverter *converter;

	/// Generate an unit converter
	virtual UnitConverter *generateUnitConverter() const = 0;

	/// Create a terminal context
	virtual bool createTerminal(TerminalContextDama **terminal,
	                            tal_id_t tal_id,
	                            rate_kbps_t cra_kbps,
	                            rate_kbps_t max_rbdc_kbps,
	                            time_sf_t rbdc_timeout_sf,
	                            vol_kb_t max_vbdc_kb);

	/// Remove a terminal context
	virtual bool removeTerminal(TerminalContextDama **terminal);

	/// Reset all terminals allocations
	virtual bool resetTerminalsAllocations();
};


#endif
