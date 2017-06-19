/*
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

bool DamaCtrlRcs::updateCarriersAndFmts()
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
		unsigned int simulated_fmt;
		unsigned int available_fmt = 0; // not in the table

		category_it = this->categories.find(terminal->getCurrentCategory());
		if(category_it == this->categories.end())
		{
			LOG(this->log_fmt, LEVEL_ERROR,
			    "SF#%u: unable to find category associated with "
			    "terminal %u\n", this->current_superframe_sf, tal_id);
			continue;
		}
		category = (*category_it).second;
		simulated_fmt = this->input_sts->getCurrentModcodId(tal_id);
		if(simulated_fmt == 0)
		{
			LOG(this->log_fmt, LEVEL_ERROR,
			    "SF#%u: cannot find MODCOD id for ST %u\n",
			    this->current_superframe_sf, tal_id);
			continue;
		}
		LOG(this->log_fmt, LEVEL_DEBUG,
		    "SF#%u: ST%u simulated FMT ID before affectation: %u\n",
		    this->current_superframe_sf, tal_id, simulated_fmt);
		// get an available MODCOD id for this terminal among carriers
		carriers_group = category->getCarriersGroups();
		for(vector<CarriersGroupDama *>::const_iterator it = carriers_group.begin();
		    it != carriers_group.end(); ++it)
		{
			CarriersGroupDama *carriers = *it;
			// FMT groups should only have one FMT id here, so get nearest should
			// return the FMT id of the carrier
			if(carriers->getNearestFmtId(simulated_fmt) == simulated_fmt)
			{
				// we have a carrier with the corresponding MODCOD
				terminal->setCarrierId(carriers->getCarriersId());
				available_fmt = simulated_fmt;
				LOG(this->log_fmt, LEVEL_DEBUG,
				    "SF#%u: ST%u will  served with the required "
				    "MODCOD (%u)\n", this->current_superframe_sf,
				    terminal->getTerminalId(), available_fmt);
				break;
			}
			// if we do not found the MODCOD value we need the closer supported value
			// MODCOD are classified from most to less robust
			if(carriers->getNearestFmtId(simulated_fmt) < simulated_fmt)
			{
				unsigned int fmt = carriers->getNearestFmtId(simulated_fmt);
				// take the closest FMT id (i.e. the bigger value)
				available_fmt = std::max(available_fmt, fmt);
				terminal->setCarrierId(carriers->getCarriersId());
			}
		}

		if(available_fmt == 0)
		{
			LOG(this->log_fmt, LEVEL_WARNING,
			    "SF#%u: cannot serve terminal %u with simulated "
			    "MODCOD %u after affectation\n",
			    this->current_superframe_sf, tal_id, simulated_fmt);
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
