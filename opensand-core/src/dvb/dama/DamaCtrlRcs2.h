/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file DamaCtrlRcs2.h
 * @brief This library defines DAMA controller interfaces.
 * @author satip6 (Eddy Fromentin)
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef _DAMA_CONTROLLER_RCS2_H_
#define _DAMA_CONTROLLER_RCS2_H_

#include "DamaCtrl.h"
#include "FmtDefinitionTable.h"
#include "TerminalContextDamaRcs.h"
#include "UnitConverter.h"

#include <opensand_output/Output.h>

#include <stdio.h>
#include <math.h>
#include <map>


/**
 * @class DamaCtrlRcs2
 * @brief Define methods to process DAMA request in the NCC
 */
class DamaCtrlRcs2: public DamaCtrl
{
public:
	DamaCtrlRcs2(spot_id_t spot);

	/**
	 * @brief  Initializes internal data structure according to configuration file
	 *
	 * @return  true on success, false otherwise
	 */
	virtual bool init();

	// Process DVB frames
	bool hereIsSAC(Rt::Ptr<Sac> sac) override;

	// Build allocation table
	bool buildTTP(Ttp &ttp) override;

	// Apply a PEP command
	virtual bool applyPepCommand(std::unique_ptr<PepRequest> request);
	
	// Update the required FMTs
	virtual void updateRequiredFmts();

	// Update wave forms
	virtual bool updateWaveForms();

protected:
	std::unique_ptr<UnitConverter> converter;

	/// Create a terminal context
	virtual bool createTerminal(std::shared_ptr<TerminalContextDama> &terminal,
	                            tal_id_t tal_id,
	                            rate_kbps_t cra_kbps,
	                            rate_kbps_t max_rbdc_kbps,
	                            time_sf_t rbdc_timeout_sf,
	                            vol_kb_t max_vbdc_kb);

	/// Remove a terminal context
	virtual bool removeTerminal(std::shared_ptr<TerminalContextDama> &terminal);

	/// Reset all terminals allocations
	virtual bool resetTerminalsAllocations();

	 ///  Reset the capacity of carriers
	virtual bool resetCarriersCapacity();

	/**
	 * @brief  Generate a probe for Gw capacity
	 *
	 * @param name            the probe name
	 * @return                the probe
	 */
	virtual std::shared_ptr<Probe<int>> generateGwCapacityProbe(std::string name) const;

	/**
	 * @brief  Generate a probe for category capacity
	 *
	 * @param name            the probe name
	 * @param category_label  the category label
	 * @return                the probe
	 */
	virtual std::shared_ptr<Probe<int>> generateCategoryCapacityProbe(
		std::string category_label,
		std::string name) const;

	/**
	 * @brief  Generate a probe for carrier capacity
	 *
	 * @param name            the probe name
	 * @param category_label  the category label
	 * @param carrier_id      the carrier id
	 * @return                the probe
	 */
	virtual std::shared_ptr<Probe<int>> generateCarrierCapacityProbe(
		std::string category_label,
		unsigned int carrier_id,
		std::string name) const;
};


#endif
