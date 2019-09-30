/*
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

/**
 * @file DamaCtrlRcs2.cpp
 * @brief This library defines a generic DAMA controller
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "DamaCtrlRcs2.h"
#include "TerminalContextDamaRcs.h"
#include "CarriersGroupDama.h"
#include "OpenSandConf.h"
#include "UnitConverterFixedSymbolLength.h"

#include <opensand_output/Output.h>
#include <opensand_conf/conf.h>

#include <math.h>


using namespace std;

/**
 * Constructor
 */
DamaCtrlRcs2::DamaCtrlRcs2(spot_id_t spot):
	DamaCtrlRcsCommon(spot)
{
}


/**
 * Destructor
 */
DamaCtrlRcs2::~DamaCtrlRcs2()
{
}

bool DamaCtrlRcs2::updateWaveForms()
{
	DamaTerminalList::iterator terminal_it;

	for(DamaTerminalList::iterator terminal_it = this->terminals.begin();
	    terminal_it != this->terminals.end(); ++terminal_it)
	{
		TerminalCategoryDama *category;
		TerminalCategories<TerminalCategoryDama>::const_iterator category_it;
		TerminalContextDamaRcs *terminal = dynamic_cast<TerminalContextDamaRcs *>(terminal_it->second);
		tal_id_t tal_id = terminal->getTerminalId();
		vector<CarriersGroupDama *> carriers_group;
		FmtDefinition *fmt_def;
		CarriersGroupDama *carriers;
		unsigned int required_fmt;
		unsigned int available_fmt = 0; // not in the table

		// get the required Fmt for the current terminal
		fmt_def = terminal->getRequiredFmt();
		required_fmt = fmt_def != NULL ? fmt_def->getId() : 0;

		// get the category
		category_it = this->categories.find(terminal->getCurrentCategory());
		if(category_it == this->categories.end())
		{
			LOG(this->log_fmt, LEVEL_ERROR,
			    "SF#%u: unable to find category associated with "
			    "terminal %u\n", this->current_superframe_sf, tal_id);
			continue;
		}
		category = (*category_it).second;
		carriers_group = category->getCarriersGroups();

		// check current carrier has the required FMT
		for(vector<CarriersGroupDama *>::iterator it = carriers_group.begin();
		    it != carriers_group.end(); ++it)
		{
			unsigned int fmt;
			// Find the terminal carrier
			carriers = *it;
			if(carriers->getCarriersId() != terminal->getCarrierId())
			{
				continue;
			}

			// Check the terminal carrier handles the required FMT
			fmt = carriers->getNearestFmtId(required_fmt);
			if(fmt != 0)
			{
				available_fmt = fmt;
				break;
			}
		}

		if(available_fmt == 0)
		{
			// get an available MODCOD id for this terminal among carriers
			for(vector<CarriersGroupDama *>::const_iterator it = carriers_group.begin();
				it != carriers_group.end(); ++it)
			{
				unsigned int fmt;
				// FMT groups should only have one FMT id here, so get nearest should
				// return the FMT id of the carrier
				carriers = *it;
				fmt = carriers->getNearestFmtId(required_fmt);
				if(required_fmt <= fmt)
				{
					// we have a carrier with the corresponding MODCOD
					terminal->setCarrierId(carriers->getCarriersId());
					available_fmt = fmt;
					LOG(this->log_fmt, LEVEL_DEBUG,
						"SF#%u: ST%u will be served with the required "
						"MODCOD (%u)\n", this->current_superframe_sf,
						terminal->getTerminalId(), available_fmt);
					break;
				}
				// if we do not found the MODCOD value we need the closer supported value
				// MODCOD are classified from most to less robust
				if(fmt < required_fmt)
				{
					// take the closest FMT id (i.e. the bigger value)
					available_fmt = std::max(available_fmt, fmt);
					terminal->setCarrierId(carriers->getCarriersId());
				}
			}
		}

		if(available_fmt == 0)
		{
			LOG(this->log_fmt, LEVEL_WARNING,
			    "SF#%u: cannot serve terminal %u with required "
			    "MODCOD %u after affectation\n",
			    this->current_superframe_sf, tal_id, required_fmt);
		}
		else
		{
			LOG(this->log_fmt, LEVEL_INFO,
			    "SF#%u: ST%u will be served with the MODCOD %u\n",
			    this->current_superframe_sf,
			    terminal->getTerminalId(), available_fmt);
		}
		// it will be 0 if the terminal cannot be served
		terminal->setFmt(this->input_modcod_def->getDefinition(available_fmt));
	}
	return true;
}

UnitConverter *DamaCtrlRcs2::generateUnitConverter() const
{
	vol_sym_t length_sym = 0;
	
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
	                   RCS2_BURST_LENGTH, length_sym))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get '%s' value", DELAY_BUFFER);
		return NULL;
	}
	if(length_sym == 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "invalid value '%u' value of '%s", length_sym, DELAY_BUFFER);
		return NULL;
	}
	LOG(this->log_init, LEVEL_INFO,
	    "Burst length = %u sym\n", length_sym);
	
	return new UnitConverterFixedSymbolLength(this->frame_duration_ms,
		0, length_sym);
}

std::shared_ptr<Probe<int>> DamaCtrlRcs2::generateGwCapacityProbe(
	string name) const
{
	char probe_name[128];

	snprintf(probe_name, sizeof(probe_name),
	         "Spot_%d.Up/Return total capacity.%s",
	         this->spot_id, name.c_str());

	return Output::Get()->registerProbe<int>(probe_name, "Sym/s", true, SAMPLE_LAST);
}

std::shared_ptr<Probe<int>> DamaCtrlRcs2::generateCategoryCapacityProbe(
	string category_label,
	string name) const
{
	char probe_name[128];

	snprintf(probe_name, sizeof(probe_name),
	         "Spot_%d.%s.Up/Return capacity.Total.%s",
	         this->spot_id, category_label.c_str(), name.c_str());

	return Output::Get()->registerProbe<int>(probe_name, "Sym/s", true, SAMPLE_LAST);
}

std::shared_ptr<Probe<int>> DamaCtrlRcs2::generateCarrierCapacityProbe(
	string category_label,
	unsigned int carrier_id,
	string name) const
{
	char probe_name[128];

	snprintf(probe_name, sizeof(probe_name),
	         "Spot_%d.%s.Up/Return capacity.Carrier%u.%s",
	         this->spot_id, category_label.c_str(), carrier_id, name.c_str());

	return Output::Get()->registerProbe<int>(probe_name, "Sym/s", true, SAMPLE_LAST);
}

bool DamaCtrlRcs2::resetCarriersCapacity()
{
	rate_symps_t gw_return_total_capacity_symps = 0;
	TerminalCategories<TerminalCategoryDama>::const_iterator category_it;
	vector<CarriersGroupDama *>::const_iterator carrier_it;

	// Initialize the capacity of carriers
	for(category_it = this->categories.begin();
	    category_it != this->categories.end();
	    ++category_it)
	{
		rate_symps_t category_return_capacity_symps = 0;
		TerminalCategoryDama *category = (*category_it).second;
		vector<CarriersGroupDama *> carriers_group = category->getCarriersGroups();
		string label = category->getLabel();

		for(carrier_it = carriers_group.begin();
		    carrier_it != carriers_group.end();
		    ++carrier_it)
		{
			CarriersGroupDama *carriers = *carrier_it;
			unsigned int carrier_id = carriers->getCarriersId();
			rate_symps_t remaining_capacity_symps;
			rate_pktpf_t remaining_capacity_pktpf;

			// we have several MODCOD for each carrier so we can't convert
			// from bauds to kbits
			remaining_capacity_symps = carriers->getTotalCapacity();
			remaining_capacity_pktpf = this->converter->symToPkt(remaining_capacity_symps);

			// initialize remaining capacity with total capacity in
			// packet per superframe as it is the unit used in DAMA computations
			carriers->setRemainingCapacity(remaining_capacity_pktpf);
			LOG(this->log_run_dama, LEVEL_NOTICE,
			    "SF#%u: Capacity before DAMA computation for "
			    "carrier %u: %u packet (per frame) (%u sym/s)\n",
			    this->current_superframe_sf,
			    carrier_id,
			    remaining_capacity_pktpf,
			    remaining_capacity_symps);

			// Output probes and stats
			// first create probes that don't exist in case of carriers
			// reallocation with SVNO interface
			if(this->probes_carrier_return_capacity[label].find(carrier_id)
			   == this->probes_carrier_return_capacity[label].end())
			{
				auto probe = this->generateCarrierCapacityProbe(label, carrier_id, "Available");
				this->probes_carrier_return_capacity[label].emplace(carrier_id, probe);
			}
			if(this->carrier_return_remaining_capacity[label].find(carrier_id)
			   == this->carrier_return_remaining_capacity[label].end())
			{
				this->carrier_return_remaining_capacity[label].emplace(carrier_id, 0);
			}
			this->probes_carrier_return_capacity[label][carrier_id]->put(remaining_capacity_symps);
			gw_return_total_capacity_symps += remaining_capacity_symps;
			category_return_capacity_symps += remaining_capacity_symps;
			this->carrier_return_remaining_capacity[label][carrier_id] = remaining_capacity_symps;
		}

		// Output probes and stats
		this->probes_category_return_capacity[label]->put(category_return_capacity_symps);
		this->category_return_remaining_capacity[label] = category_return_capacity_symps;
	}

	//Output probes and stats
	this->probe_gw_return_total_capacity->put(gw_return_total_capacity_symps);
	this->gw_remaining_capacity = gw_return_total_capacity_symps;

	return true;
}

