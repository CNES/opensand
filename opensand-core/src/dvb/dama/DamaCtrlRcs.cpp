/*
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

/**
 * @file DamaCtrlRcs.cpp
 * @brief This library defines a generic DAMA controller
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieutoulouse.viveris.com>
 */


#include "DamaCtrlRcs.h"
#include "TerminalContextDamaRcs.h"
#include "CarriersGroupDama.h"
#include "UnitConverterFixedBitLength.h"

#include <opensand_output/Output.h>

#include <math.h>


using namespace std;

/**
 * Constructor
 */
DamaCtrlRcs::DamaCtrlRcs(spot_id_t spot, vol_b_t packet_length_b):
	DamaCtrlRcsCommon(spot),
	packet_length_b(packet_length_b)
{
}


/**
 * Destructor
 */
DamaCtrlRcs::~DamaCtrlRcs()
{
}

bool DamaCtrlRcs::updateWaveForms()
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

		// get an available MODCOD id for this terminal among carriers
		for(vector<CarriersGroupDama *>::const_iterator it = carriers_group.begin();
		    it != carriers_group.end(); ++it)
		{
			CarriersGroupDama *carriers = *it;
			unsigned int fmt = carriers->getNearestFmtId(required_fmt);
			// FMT groups should only have one FMT id here, so get nearest should
			// return the FMT id of the carrier
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

UnitConverter *DamaCtrlRcs::generateUnitConverter() const
{
	return new UnitConverterFixedBitLength(this->frame_duration_ms,
		0, this->packet_length_b);
}

Probe<int> *DamaCtrlRcs::generateGwCapacityProbe(
	string name) const
{
	char probe_name[128];

	snprintf(probe_name, sizeof(probe_name),
	         "Spot_%d.Up/Return total capacity.%s",
	         this->spot_id, name.c_str());

	return Output::registerProbe<int>(probe_name, "Kbits/s", true, SAMPLE_LAST);
}

Probe<int> *DamaCtrlRcs::generateCategoryCapacityProbe(
	string category_label,
	string name) const
{
	char probe_name[128];

	snprintf(probe_name, sizeof(probe_name),
	         "Spot_%d.%s.Up/Return capacity.Total.%s",
	         this->spot_id, category_label.c_str(), name.c_str());

	return Output::registerProbe<int>(probe_name, "Kbits/s", true, SAMPLE_LAST);
}

Probe<int> *DamaCtrlRcs::generateCarrierCapacityProbe(
	string category_label,
	unsigned int carrier_id,
	string name) const
{
	char probe_name[128];

	snprintf(probe_name, sizeof(probe_name),
	         "Spot_%d.%s.Up/Return capacity.Carrier%u.%s",
	         this->spot_id, category_label.c_str(), carrier_id, name.c_str());

	return Output::registerProbe<int>(probe_name, "Kbits/s", true, SAMPLE_LAST);
}

bool DamaCtrlRcs::resetCarriersCapacity()
{
	rate_kbps_t gw_return_total_capacity_kbps = 0;
	TerminalCategories<TerminalCategoryDama>::const_iterator category_it;
	vector<CarriersGroupDama *>::const_iterator carrier_it;

	// Initialize the capacity of carriers
	for(category_it = this->categories.begin();
	    category_it != this->categories.end();
	    ++category_it)
	{
		rate_kbps_t category_return_capacity_kbps = 0;
		TerminalCategoryDama *category = (*category_it).second;
		vector<CarriersGroupDama *> carriers_group = category->getCarriersGroups();
		string label = category->getLabel();

		for(carrier_it = carriers_group.begin();
		    carrier_it != carriers_group.end();
		    ++carrier_it)
		{
			CarriersGroupDama *carriers = *carrier_it;
			unsigned int carrier_id = carriers->getCarriersId();
			vol_kb_t remaining_capacity_kb;
			rate_kbps_t remaining_capacity_kbps;
			rate_pktpf_t remaining_capacity_pktpf;

			// we have only one MODCOD for each carrier so we can convert
			// directly from bauds to kbits
			remaining_capacity_kb =
				this->input_modcod_def->symToKbits(carriers->getFmtIds().front(),
				                       carriers->getTotalCapacity());
			remaining_capacity_kbps = this->converter->pfToPs(remaining_capacity_kb);
			remaining_capacity_pktpf = this->converter->kbitsToPkt(remaining_capacity_kb);

			// initialize remaining capacity with total capacity in
			// packet per superframe as it is the unit used in DAMA computations
			carriers->setRemainingCapacity(remaining_capacity_pktpf);
			LOG(this->log_run_dama, LEVEL_NOTICE,
			    "SF#%u: Capacity before DAMA computation for "
			    "carrier %u: %u packet (per frame) (%u kb/s)\n",
			    this->current_superframe_sf,
			    carrier_id,
			    remaining_capacity_pktpf,
			    remaining_capacity_kbps);

			// Output probes and stats
			// first create probes that don't exist in case of carriers
			// reallocation with SVNO interface
			if(this->probes_carrier_return_capacity[label].find(carrier_id)
			   == this->probes_carrier_return_capacity[label].end())
			{
				Probe<int> *probe = this->generateCarrierCapacityProbe(
					label,
					carrier_id,
					"Available");
				this->probes_carrier_return_capacity[label].insert(
				    std::pair<unsigned int,Probe<int> *>(carrier_id, probe));
			}
			if(this->carrier_return_remaining_capacity[label].find(carrier_id)
			   == this->carrier_return_remaining_capacity[label].end())
			{
				this->carrier_return_remaining_capacity[label].insert(
				    std::pair<unsigned int, int>(carrier_id, 0));
			}
			this->probes_carrier_return_capacity[label][carrier_id]
				->put(remaining_capacity_kbps);
			gw_return_total_capacity_kbps += remaining_capacity_kbps;
			category_return_capacity_kbps += remaining_capacity_kbps;
			this->carrier_return_remaining_capacity[label][carrier_id] = remaining_capacity_kbps;
		}

		// Output probes and stats
		this->probes_category_return_capacity[label]->put(category_return_capacity_kbps);
		this->category_return_remaining_capacity[label] = category_return_capacity_kbps;
	}

	//Output probes and stats
	this->probe_gw_return_total_capacity->put(gw_return_total_capacity_kbps);
	this->gw_remaining_capacity = gw_return_total_capacity_kbps;

	return true;
}

