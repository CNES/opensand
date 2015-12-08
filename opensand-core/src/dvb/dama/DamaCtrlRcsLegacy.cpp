/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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
 * @file DamaCtrlRcsLegacy.cpp
 * @brief This library defines Legacy DAMA controller
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */


#include "DamaCtrlRcsLegacy.h"

#include "OpenSandFrames.h"
#include "UnitConverter.h"

#include <opensand_output/Output.h>

#include <math.h>
#include <string>


/**
 * Constructor
 */
DamaCtrlRcsLegacy::DamaCtrlRcsLegacy(spot_id_t spot): DamaCtrlRcs(spot)
{
}


/**
 * Destructor
 */
DamaCtrlRcsLegacy::~DamaCtrlRcsLegacy()
{
}

bool DamaCtrlRcsLegacy::init()
{
	TerminalCategories<TerminalCategoryDama>::const_iterator category_it;
	vector<CarriersGroupDama *>::const_iterator carrier_it;

	if(!DamaCtrlRcs::init())
	{
		return false;
	}

	// check that we have only one MODCOD per carrier
	for(category_it = this->categories.begin();
	    category_it != this->categories.end();
	    ++category_it)
	{
		TerminalCategoryDama *category = (*category_it).second;
		vector<CarriersGroupDama *> carriers_group;
		string label = category->getLabel();

		carriers_group = category->getCarriersGroups();
		for(carrier_it = carriers_group.begin();
		    carrier_it != carriers_group.end();
		    ++carrier_it)
		{
			CarriersGroupDama *carriers = *carrier_it;
			if(carriers->getFmtIds().size() > 1)
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "you should only define one FMT ID per FMT "
				    "group for Legacy DAMA\n");
				return false;
			}
			// Output probes and stats
			Probe<int> *probe_carrier_capacity;
			Probe<int> *probe_carrier_remaining_capacity;
			unsigned int carrier_id = carriers->getCarriersId();
			probe_carrier_capacity = Output::registerProbe<int>("Kbits/s",
				true, SAMPLE_LAST, "Spot_%d.%s.Up/Return capacity.Carrier%u.Available",
				this->spot_id, label.c_str(), carrier_id);

			probe_carrier_remaining_capacity = Output::registerProbe<int>("Kbits/s",
				true, SAMPLE_LAST, "Spot_%d.%s.Up/Return capacity.Carrier%u.Remaining",
				this->spot_id, label.c_str(), carrier_id);

			this->probes_carrier_return_capacity[label].insert(
				std::pair<unsigned int,Probe<int> *>(carrier_id,
				                                     probe_carrier_capacity));

			this->probes_carrier_return_remaining_capacity[label].insert(
				std::pair<unsigned int, Probe<int> *>(carrier_id,
				                                      probe_carrier_remaining_capacity));

			this->carrier_return_remaining_capacity_pktpf[label].insert(
				std::pair<unsigned int, int>(carrier_id, 0));
		}
		// Output probes and stats
		Probe<int> *probe_category_capacity;
		Probe<int> *probe_category_remaining_capacity;

		probe_category_capacity = Output::registerProbe<int>(
				"Kbits/s", true, SAMPLE_LAST,
				"Spot_%d.%s.Up/Return capacity.Total.Available",
				this->spot_id,
				label.c_str());
		this->probes_category_return_capacity.insert(
			std::pair<string,Probe<int> *>(label, probe_category_capacity));

		probe_category_remaining_capacity = Output::registerProbe<int>(
			"Kbits/s", true,  SAMPLE_LAST,
			"Spot_%d.%s.Up/Return capacity.Total.Remaining",
			this->spot_id,
			label.c_str());
		this->probes_category_return_remaining_capacity.insert(
			std::pair<string,Probe<int> *>(label, probe_category_remaining_capacity));

		this->category_return_remaining_capacity_pktpf.insert(
			std::pair<string, int>(label, 0));
	}

	return true;
}

bool DamaCtrlRcsLegacy::runDamaRbdc()
{
	TerminalCategories<TerminalCategoryDama>::const_iterator category_it;
	vector<CarriersGroupDama *>::const_iterator carrier_it;

	for(category_it = this->categories.begin();
	    category_it != this->categories.end();
	    ++category_it)
	{
		TerminalCategoryDama *category = (*category_it).second;
		vector<CarriersGroupDama *> carriers_group;

		// we ca compute RBDC per carriers group because a terminal
		// is assigned to one on each frame, depending on its DRA
		carriers_group = category->getCarriersGroups();
		for(carrier_it = carriers_group.begin();
		    carrier_it != carriers_group.end();
		    ++carrier_it)
		{
			this->runDamaRbdcPerCarrier(*carrier_it,
			                            category);
		}
	}
	// Output stats and probes
	this->probe_gw_rbdc_req_num->put(gw_rbdc_req_num);
	this->gw_rbdc_req_num = 0;
	this->probe_gw_rbdc_req_size->put(
		this->converter->pktpfToKbps(this->gw_rbdc_req_size_pktpf));
	this->gw_rbdc_req_size_pktpf = 0;
	this->probe_gw_rbdc_alloc->put(
		this->converter->pktpfToKbps(this->gw_rbdc_alloc_pktpf));
	this->gw_rbdc_alloc_pktpf = 0;

	return true;
}


bool DamaCtrlRcsLegacy::runDamaVbdc()
{
	TerminalCategories<TerminalCategoryDama>::const_iterator category_it;
	vector<CarriersGroupDama *>::const_iterator carrier_it;

	for(category_it = this->categories.begin();
	    category_it != this->categories.end();
	    ++category_it)
	{
		TerminalCategoryDama *category = (*category_it).second;
		vector<CarriersGroupDama *> carriers_group;

		carriers_group = category->getCarriersGroups();
		for(carrier_it = carriers_group.begin();
		    carrier_it != carriers_group.end();
		    ++carrier_it)
		{
			this->runDamaVbdcPerCarrier(*carrier_it,
			                            category);
		}
	}

	// Output stats and probes
	this->probe_gw_vbdc_req_num->put(this->gw_vbdc_req_num);
	this->gw_vbdc_req_num = 0;
	this->probe_gw_vbdc_req_size->put(
		this->converter->pktToKbits(this->gw_vbdc_req_size_pkt));
	this->gw_vbdc_req_size_pkt = 0;
	this->probe_gw_vbdc_alloc->put(
		this->converter->pktToKbits(this->gw_vbdc_alloc_pkt));
	this->gw_vbdc_alloc_pkt = 0;

	return true;
}


bool DamaCtrlRcsLegacy::runDamaFca()
{
	TerminalCategories<TerminalCategoryDama>::const_iterator category_it;
	vector<CarriersGroupDama *>::const_iterator carrier_it;

	if(this->fca_kbps == 0)
	{
		LOG(this->log_run_dama, LEVEL_INFO,
		    "SF#%u: no fca, skip\n", this->current_superframe_sf);
		return true;
	}

	for(category_it = this->categories.begin();
	    category_it != this->categories.end();
	    ++category_it)
	{
		TerminalCategoryDama *category = (*category_it).second;
		vector<CarriersGroupDama *> carriers_group;

		carriers_group = category->getCarriersGroups();
		for(carrier_it = carriers_group.begin();
		    carrier_it != carriers_group.end();
		    ++carrier_it)
		{
			this->runDamaFcaPerCarrier(*carrier_it,
			                            category);
		}
	}

	// Be careful to use probes only if FCA is enabled
	// Output probes and stats
	this->probe_gw_fca_alloc->put(
		this->converter->pktpfToKbps(this->gw_fca_alloc_pktpf));
	this->gw_fca_alloc_pktpf = 0;

	return true;
}


bool DamaCtrlRcsLegacy::resetDama()
{
	TerminalCategories<TerminalCategoryDama>::const_iterator category_it;
	vector<CarriersGroupDama *>::const_iterator carrier_it;

	// Initialize the capacity of carriers
	for(category_it = this->categories.begin();
	    category_it != this->categories.end();
	    ++category_it)
	{
		TerminalCategoryDama *category = (*category_it).second;
		vector<CarriersGroupDama *> carriers_group;
		string label = category->getLabel();

		carriers_group = category->getCarriersGroups();
		for(carrier_it = carriers_group.begin();
		    carrier_it != carriers_group.end();
		    ++carrier_it)
		{
			CarriersGroupDama *carriers = *carrier_it;
			unsigned int carriers_id = carriers->getCarriersId();
			vol_kb_t remaining_capacity_kb;
			rate_pktpf_t remaining_capacity_pktpf;
			// we have only one MODCOD for each carrier so we can convert
			// directly from bauds to kbits
			remaining_capacity_kb =
				this->input_modcod_def->symToKbits(carriers->getFmtIds().front(),
				                       carriers->getTotalCapacity());
			// as this function is called each superframe we can directly
			// convert number of packet to rate in packet per superframe
			// and dividing by the frame number per superframes we have
			// the rate in packet per frame
			remaining_capacity_pktpf =
				this->converter->kbitsToPkt(remaining_capacity_kb);


			// initialize remaining capacity with total capacity in
			// packet per superframe as it is the unit used in DAMA computations
			carriers->setRemainingCapacity(remaining_capacity_pktpf);
			LOG(this->log_run_dama, LEVEL_NOTICE,
			    "SF#%u: Capacity before DAMA computation for "
			    "carrier %u: %u packet (per frame) (%u kb)\n",
			    this->current_superframe_sf,
			    carriers_id,
			    remaining_capacity_pktpf,
			    remaining_capacity_kb);

			// Output probes and stats
			// first create probes that don't exist in case of carriers
			// reallocation with SVNO interface
			if(this->probes_carrier_return_capacity[label].find(carriers_id)
			   == this->probes_carrier_return_capacity[label].end())
			{
				Probe<int> *probe_carrier_capacity;
				probe_carrier_capacity = Output::registerProbe<int>("Kbits/s",
				    true, SAMPLE_LAST, "Spot_%d.%s.Up/Return capacity.Carrier%u.Available",
				    this->spot_id, label.c_str(), carriers_id);
				this->probes_carrier_return_capacity[label].insert(
				    std::pair<unsigned int,Probe<int> *>(carriers_id,
				                                         probe_carrier_capacity));
			}
			if(this->carrier_return_remaining_capacity_pktpf[label].find(carriers_id)
			   == this->carrier_return_remaining_capacity_pktpf[label].end())
			{
				this->carrier_return_remaining_capacity_pktpf[label].insert(
				    std::pair<unsigned int, int>(carriers_id, 0));
			}

			this->probes_carrier_return_capacity[label][carriers_id]
				->put(this->converter->pktpfToKbps(remaining_capacity_pktpf));
			this->gw_return_total_capacity_pktpf += remaining_capacity_pktpf;
			this->category_return_capacity_pktpf += remaining_capacity_pktpf;
			this->carrier_return_remaining_capacity_pktpf[label][carriers_id] =
				remaining_capacity_pktpf;
		}

		// Output probes and stats
		this->probes_category_return_capacity[label]->put(
			this->converter->pktpfToKbps(category_return_capacity_pktpf));
		this->category_return_remaining_capacity_pktpf[label] =
			category_return_capacity_pktpf;
		this->category_return_capacity_pktpf = 0;
	}

	//Output probes and stats
	this->probe_gw_return_total_capacity->put(
		this->converter->pktpfToKbps(gw_return_total_capacity_pktpf));
	this->gw_remaining_capacity_pktpf = gw_return_total_capacity_pktpf;
	this->gw_return_total_capacity_pktpf = 0;

	return true;
}


void DamaCtrlRcsLegacy::runDamaRbdcPerCarrier(CarriersGroupDama *carriers,
                                              const TerminalCategoryDama *category)
{
	rate_pktpf_t total_request_pktpf = 0;
	double fair_share;
	double fair_rbdc_pktpf;
	rate_pktpf_t rbdc_alloc_pktpf = 0;
	vector<TerminalContextDamaRcs *> tal;
	TerminalContextDamaRcs *terminal;
	unsigned int carrier_id = carriers->getCarriersId();
	rate_pktpf_t remaining_capacity_pktpf;
	vector<TerminalContextDamaRcs *>::iterator tal_it;
	int simu_rbdc = 0;
	ostringstream buf;
	string label = category->getLabel();
	string debug;
	buf << "SF#" << this->current_superframe_sf << " carrier "
	    << carrier_id << ", category " << label << ":";
	debug = buf.str();

	remaining_capacity_pktpf = carriers->getRemainingCapacity();

	if(remaining_capacity_pktpf == 0)
	{
		LOG(this->log_run_dama, LEVEL_INFO,
		    "%s skipping RBDC dama computation: Not enough "
		    "capacity\n", debug.c_str());
		return;
	}

	LOG(this->log_run_dama, LEVEL_INFO,
	    "%s remaining capacity = %u pktpf before RBDC allocation \n",
	    debug.c_str(), remaining_capacity_pktpf);

	tal = category->getTerminalsInCarriersGroup<TerminalContextDamaRcs>(carrier_id);
	// get total RBDC requests
	for(tal_it = tal.begin(); tal_it != tal.end(); ++tal_it)
	{
		rate_pktpf_t request_pktpf;
		terminal = *tal_it;
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%d: RBDC request %d packet per superframe\n",
		    debug.c_str(), terminal->getTerminalId(),
		    terminal->getRequiredRbdc());

		request_pktpf = terminal->getRequiredRbdc();
		total_request_pktpf += request_pktpf;

		// Output stats and probes
		if (request_pktpf > 0)
			this->gw_rbdc_req_num++;
	}
	// Output stats and probes
	gw_rbdc_req_size_pktpf += total_request_pktpf;

	if(total_request_pktpf == 0)
	{
		LOG(this->log_run_dama, LEVEL_INFO,
		    "%s no RBDC request for this frame.\n", debug.c_str());

		// Output stats and probes
		for(tal_it = tal.begin(); tal_it != tal.end(); ++tal_it)
		{
			terminal = *tal_it;
			tal_id_t tal_id = terminal->getTerminalId();
			if(tal_id < BROADCAST_TAL_ID)
			{
				this->probes_st_rbdc_alloc[tal_id]->put(0);
			}
		}
		if(this->simulated)
		{
			this->probes_st_rbdc_alloc[0]->put(0);
		}

		return;
	}

	// Fair share calculation
	fair_share = (double) total_request_pktpf / remaining_capacity_pktpf;

	// if there is no congestion,
	// force the ratio to 1.0 in order to avoid requests limitation
	if(fair_share < 1.0)
	{
		fair_share = 1.0;

		// Output probes and stats
		this->gw_rbdc_alloc_pktpf += total_request_pktpf;
	}
	else
	{
		// Output probes and stats
		this->gw_rbdc_alloc_pktpf += remaining_capacity_pktpf;
	}

	LOG(this->log_run_dama, LEVEL_INFO,
	    "%s: sum of all RBDC requests = %u packets per superframe "
	    " -> Fair share=%f\n", debug.c_str(),
	    total_request_pktpf, fair_share);

	// first step : serve the integer part of the fair RBDC
	for(tal_it = tal.begin(); tal_it != tal.end(); ++tal_it)
	{
		terminal = *tal_it;
		tal_id_t tal_id = terminal->getTerminalId();

		// apply the fair share coef to all requests
		fair_rbdc_pktpf = (double) (terminal->getRequiredRbdc() / fair_share);

		// take the integer part of fair RBDC
		rbdc_alloc_pktpf = floor(fair_rbdc_pktpf);
		terminal->setRbdcAllocation(rbdc_alloc_pktpf);
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%u RBDC alloc %u packets per superframe\n",
		    debug.c_str(), tal_id, rbdc_alloc_pktpf);

		// decrease the total capacity
		remaining_capacity_pktpf -= rbdc_alloc_pktpf;

		// Output probes and stats
		if(tal_id > BROADCAST_TAL_ID)
		{
			simu_rbdc += rbdc_alloc_pktpf;
		}
		else
		{
			this->probes_st_rbdc_alloc[tal_id]->put(
				this->converter->pktpfToKbps(rbdc_alloc_pktpf));
		}
		this->carrier_return_remaining_capacity_pktpf[label][carrier_id] -= rbdc_alloc_pktpf;
		this->category_return_remaining_capacity_pktpf[label]
			-= rbdc_alloc_pktpf;
		this->gw_remaining_capacity_pktpf -= rbdc_alloc_pktpf;

		if(fair_share > 1.0)
		{
			// add the decimal part of the fair RBDC
			terminal->addRbdcCredit(fair_rbdc_pktpf - rbdc_alloc_pktpf);
		}
	}
	if(this->simulated)
	{
		this->probes_st_rbdc_alloc[0]->put(
				this->converter->pktpfToKbps(simu_rbdc));
	}

	// second step : RBDC decimal part treatment
	if(fair_share > 1.0)
	{
		// sort terminal according to their remaining credit
		std::stable_sort(tal.begin(), tal.end(),
		                 TerminalContextDamaRcs::sortByRemainingCredit);
		for(tal_it = tal.begin(); tal_it != tal.end(); ++tal_it)
		{
			rate_pktpf_t credit_pktpf;
			terminal = *tal_it;
			tal_id_t tal_id = terminal->getTerminalId();

			if(remaining_capacity_pktpf == 0)
			{
				break;
			}
			credit_pktpf = terminal->getRbdcCredit();
			LOG(this->log_run_dama, LEVEL_DEBUG,
			    "%s step 2 scanning ST%u remaining capacity=%u "
			    "credit_pktpf=%u\n", debug.c_str(),
			    tal_id, remaining_capacity_pktpf, credit_pktpf);
			if(credit_pktpf > 1.0)
			{
				if(terminal->getMaxRbdc() - rbdc_alloc_pktpf > 1)
				{
					// enough capacity to allocate
					terminal->setRbdcAllocation(rbdc_alloc_pktpf + 1);
					terminal->addRbdcCredit(-1);
					remaining_capacity_pktpf--;
					LOG(this->log_run_dama, LEVEL_DEBUG,
					    "%s step 2 allocating 1 cell to ST%u\n",
					    debug.c_str(), tal_id);
					// Update probes and stats
					this->carrier_return_remaining_capacity_pktpf[label][carrier_id]--;
					this->category_return_remaining_capacity_pktpf
						[label]--;
					this->gw_remaining_capacity_pktpf--;
				}
			}
		}
	}
	carriers->setRemainingCapacity(remaining_capacity_pktpf);
}


void DamaCtrlRcsLegacy::runDamaVbdcPerCarrier(CarriersGroupDama *carriers,
                                              const TerminalCategoryDama *category)
{
	vector<TerminalContextDamaRcs *> tal;
	TerminalContextDamaRcs *terminal;
	unsigned int carrier_id = carriers->getCarriersId();
	rate_pktpf_t remaining_capacity_pktpf;
	vector<TerminalContextDamaRcs *>::iterator tal_it;
	int simu_vbdc = 0;
	ostringstream buf;
	string label = category->getLabel();
	string debug;
	buf << "SF#" << this->current_superframe_sf << " carrier "
	    << carrier_id << ", category " << label << ":";
	debug = buf.str();

	remaining_capacity_pktpf = carriers->getRemainingCapacity();

	tal = category->getTerminalsInCarriersGroup<TerminalContextDamaRcs>(carrier_id);
	if(remaining_capacity_pktpf == 0)
	{
		LOG(this->log_run_dama, LEVEL_NOTICE,
		    "%s skipping VBDC dama computation: Not enough "
		    "capacity\n", debug.c_str());

		// Output stats and probes
		for(tal_it = tal.begin(); tal_it != tal.end(); ++tal_it)
		{
			terminal = *tal_it;
			tal_id_t tal_id = terminal->getTerminalId();
			if(tal_id < BROADCAST_TAL_ID)
			{
				this->probes_st_vbdc_alloc[tal_id]->put(0);
			}
		}
		if(this->simulated)
		{
			this->probes_st_vbdc_alloc[0]->put(0);
		}

		return;
	}

	LOG(this->log_run_dama, LEVEL_INFO,
	    "%s remaining capacity = %u packets before VBDC "
	    "allocation \n", debug.c_str(), remaining_capacity_pktpf);

	// sort terminal according to their VBDC requests
	std::stable_sort(tal.begin(), tal.end(),
	                 TerminalContextDamaRcs::sortByVbdcReq);
	tal_it = tal.begin();
	if(tal_it == tal.end())
	{
		// no ST
		return;
	}

	// try to serve the required VBDC
	// the setVbdcAllocation functions had updated the VBDC requests
	// sort terminal according to their new VBDC requests
	std::stable_sort(tal.begin(), tal.end(),
	                 TerminalContextDamaRcs::sortByVbdcReq);
	for(tal_it = tal.begin(); tal_it != tal.end(); ++tal_it)
	{
		terminal = *tal_it;
		tal_id_t tal_id = terminal->getTerminalId();

		vol_pkt_t request_pkt = terminal->getRequiredVbdc();

		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s: ST%u remaining capacity=%u remaining VBDC "
		    "request %u\n", debug.c_str(),
		    tal_id, remaining_capacity_pktpf, request_pkt);

		if(request_pkt > 0)
		{

			// Output stats and probes
			if(this->probe_gw_vbdc_req_size->isEnabled() ||
			this->probe_gw_vbdc_req_num->isEnabled())
			{
				this->gw_vbdc_req_num++;
				this->gw_vbdc_req_size_pkt += request_pkt;
			}

			if(request_pkt <= remaining_capacity_pktpf)
			{
				// enough capacity to allocate
				remaining_capacity_pktpf -= request_pkt;
				terminal->setVbdcAllocation(request_pkt);
				LOG(this->log_run_dama, LEVEL_DEBUG,
				    "%s ST%u allocate remaining VBDC: %u\n",
				    debug.c_str(), tal_id, request_pkt);

				// Output probes and stats
				if(tal_id > BROADCAST_TAL_ID)
				{
					simu_vbdc += request_pkt;
				}
				else
				{
					this->probes_st_vbdc_alloc[tal_id]->put(
						this->converter->pktToKbits(request_pkt));
				}
				this->gw_vbdc_alloc_pkt += request_pkt;
				this->carrier_return_remaining_capacity_pktpf[label][carrier_id] -=
					request_pkt;
				this->category_return_remaining_capacity_pktpf[label]
					-= request_pkt;
				this->gw_remaining_capacity_pktpf -= request_pkt;
			}
			else
			{
				// not enough capacity to allocate the complete request
				terminal->setVbdcAllocation(remaining_capacity_pktpf);

				// Output stats and probes
				if(tal_id > BROADCAST_TAL_ID)
				{
					simu_vbdc += remaining_capacity_pktpf;
				}
				else
				{
					this->probes_st_vbdc_alloc[tal_id]->put(
						this->converter->pktToKbits(remaining_capacity_pktpf));
				}
				if(this->probe_gw_vbdc_req_size->isEnabled() ||
					this->probe_gw_vbdc_req_num->isEnabled() ||
					this->probe_gw_vbdc_alloc->isEnabled())
				{
					this->gw_vbdc_alloc_pkt += remaining_capacity_pktpf;
					do
					{
						terminal = *tal_it;
						request_pkt = terminal->getRequiredVbdc();
						gw_vbdc_req_size_pkt += request_pkt;
						if(request_pkt > 0)
							this->gw_vbdc_req_num++;
						tal_it++;
					}
					while(tal_it != tal.end());
				}
				this->carrier_return_remaining_capacity_pktpf[label][carrier_id] -=
					remaining_capacity_pktpf;
				this->category_return_remaining_capacity_pktpf[label]
					-= remaining_capacity_pktpf;
				this->gw_remaining_capacity_pktpf -= remaining_capacity_pktpf;

				LOG(this->log_run_dama, LEVEL_DEBUG,
				    "%s: ST%u allocate partial remaining VBDC: "
				    "%u<%u\n", debug.c_str(),
				    tal_id, remaining_capacity_pktpf, request_pkt);
				remaining_capacity_pktpf = 0;

				return;
			}
		}
	}
	if(this->simulated)
	{
		this->probes_st_vbdc_alloc[0]->put(
				this->converter->pktToKbits(simu_vbdc));
	}

	carriers->setRemainingCapacity(remaining_capacity_pktpf);
}


// TODO it would be better if, at the end of allocations computation,
//      we try to move some terminals not totally served in supported carriers
//      (in the same category and with supported MODCOD value) in which there
//      is still capacity
void DamaCtrlRcsLegacy::runDamaFcaPerCarrier(CarriersGroupDama *carriers,
                                             const TerminalCategoryDama *category)
{
	vector<TerminalContextDamaRcs *> tal;
	TerminalContextDamaRcs *terminal;
	unsigned int carrier_id = carriers->getCarriersId();
	rate_pktpf_t remaining_capacity_pktpf;
	rate_pktpf_t fca_pktpf;
	int simu_fca = 0;
	vector<TerminalContextDamaRcs *>::iterator tal_it;
	ostringstream buf;
	string label = category->getLabel();
	string debug;
	buf << "SF#" << this->current_superframe_sf << " carrier "
	    << carrier_id << ", category " << label << ":";
	debug = buf.str();

	fca_pktpf = this->converter->kbpsToPktpf(this->fca_kbps);

	tal = category->getTerminalsInCarriersGroup<TerminalContextDamaRcs>(carrier_id);
	tal_it = tal.begin();
	if(tal_it == tal.end())
	{
		// no ST
		return;
	}

	remaining_capacity_pktpf = carriers->getRemainingCapacity();

	if(remaining_capacity_pktpf <= 0)
	{
		// Be careful to use probes only if FCA is enabled
		// Output probes and stats
		while(tal_it != tal.end())
		{
			tal_id_t tal_id = (*tal_it)->getTerminalId();
			if(tal_id < BROADCAST_TAL_ID)
			{
				this->probes_st_fca_alloc[tal_id]->put(0);
			}
			tal_it++;
		}
		if(this->simulated)
		{
			this->probes_st_fca_alloc[0]->put(0);
		}

		LOG(this->log_run_dama, LEVEL_NOTICE,
		    "%s skipping FCA dama computaiton. Not enough "
		    "capacity\n", debug.c_str());
		return;
	}

	LOG(this->log_run_dama, LEVEL_INFO,
	    "%s remaining capacity = %u packets before FCA "
	    "computation\n", debug.c_str(), remaining_capacity_pktpf);

	// sort terminal according to their remaining credit
	// this is a random but logical choice
	std::stable_sort(tal.begin(), tal.end(),
	                 TerminalContextDamaRcs::sortByRemainingCredit);

	while(tal_it != tal.end())
	{
		terminal = *tal_it;
		tal_id_t tal_id = terminal->getTerminalId();

		if (remaining_capacity_pktpf > fca_pktpf)
		{
			remaining_capacity_pktpf -= fca_pktpf;
			LOG(this->log_run_dama, LEVEL_DEBUG,
			    "%s ST%u FCA allocation %u)\n", debug.c_str(),
			    tal_id, fca_pktpf);
			terminal->setFcaAllocation(fca_pktpf);

			// Output probes and stats
			if(tal_id > BROADCAST_TAL_ID)
			{
				simu_fca += fca_pktpf;
			}
			else
			{
				this->probes_st_fca_alloc[tal_id]->put(
					this->converter->pktpfToKbps(fca_pktpf));
			}
			this->carrier_return_remaining_capacity_pktpf[label][carrier_id] -=
				fca_pktpf;
			this->category_return_remaining_capacity_pktpf[label]
				-= fca_pktpf;
			this->gw_remaining_capacity_pktpf -= fca_pktpf;
		}
		else
		{
			LOG(this->log_run_dama, LEVEL_DEBUG,
			    "%s ST%u FCA allocation %u)\n",
			    debug.c_str(), tal_id, remaining_capacity_pktpf);
			terminal->setFcaAllocation(remaining_capacity_pktpf);

			// Output probes and stats
			if(tal_id > BROADCAST_TAL_ID)
			{
				simu_fca += fca_pktpf;
			}
			else
			{
				this->probes_st_fca_alloc[tal_id]->put(
					this->converter->pktpfToKbps(remaining_capacity_pktpf));
			}
			this->carrier_return_remaining_capacity_pktpf[label][carrier_id] -=
				remaining_capacity_pktpf;
			this->category_return_remaining_capacity_pktpf[label]
				-= remaining_capacity_pktpf;
			this->gw_remaining_capacity_pktpf -= remaining_capacity_pktpf;

			remaining_capacity_pktpf = 0;
		}

		// Output probes and stats
		this->gw_fca_alloc_pktpf += terminal->getFcaAllocation();

		tal_it++;
	}
	if(this->simulated)
	{
		this->probes_st_fca_alloc[0]->put(
			this->converter->pktpfToKbps(simu_fca));
	}

	carriers->setRemainingCapacity(remaining_capacity_pktpf);
}


