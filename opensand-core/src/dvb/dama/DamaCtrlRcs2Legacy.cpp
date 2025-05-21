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
 * @file DamaCtrlRcs2Legacy.cpp
 * @brief This library defines Legacy DAMA controller
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "DamaCtrlRcs2Legacy.h"

#include "OpenSandFrames.h"
#include "TerminalContextDamaRcs.h"

#include <opensand_output/Output.h>

#include <math.h>
#include <string>
#include <sstream>


/**
 * Constructor
 */
DamaCtrlRcs2Legacy::DamaCtrlRcs2Legacy(spot_id_t spot):
	DamaCtrlRcs2(spot)
{
}


/**
 * Destructor
 */
DamaCtrlRcs2Legacy::~DamaCtrlRcs2Legacy()
{
}

bool DamaCtrlRcs2Legacy::init()
{
	if(!DamaCtrlRcs2::init())
	{
		return false;
	}

	// check that we have only one MODCOD per carrier
	for (auto &&category_it: this->categories)
	{
		std::shared_ptr<TerminalCategoryDama> category = category_it.second;
		std::string label = category->getLabel();

		for (auto &&carriers: category->getCarriersGroups())
		{
			if(carriers.getCarriersNumber() > 1)
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "you should only define one carrier per category "
				    "for DVB-RCS2 Legacy DAMA\n");
				return false;
			}
			if(carriers.getFmtIds().size() == 0)
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "you should define at least one FMT ID per FMT "
				    "group for DVB-RCS2 Legacy DAMA\n");
				return false;
			}
			// Output probes and stats
			std::shared_ptr<Probe<int>> probe_carrier;
			unsigned int carrier_id = carriers.getCarriersId();
			probe_carrier = this->generateCarrierCapacityProbe(label, carrier_id, "Available");
			this->probes_carrier_return_capacity[label].emplace(carrier_id, probe_carrier);

			probe_carrier = this->generateCarrierCapacityProbe(label, carrier_id, "Remaining");
			this->probes_carrier_return_remaining_capacity[label].emplace(carrier_id, probe_carrier);

			this->carrier_return_remaining_capacity[label].emplace(carrier_id, 0);
		}
		// Output probes and stats
		std::shared_ptr<Probe<int>> probe_category;

		probe_category = this->generateCategoryCapacityProbe(label, "Available");
		this->probes_category_return_capacity.emplace(label, probe_category);

		probe_category = this->generateCategoryCapacityProbe(label, "Remaining");
		this->probes_category_return_remaining_capacity.emplace(label, probe_category);

		this->category_return_remaining_capacity.emplace(label, 0);
	}

	return true;
}

bool DamaCtrlRcs2Legacy::computeTerminalsCraAllocation()
{
	bool stat = true;
	rate_kbps_t gw_cra_request_kbps = 0;

	this->gw_cra_alloc_kbps = 0;
	for (auto &&category_it: this->categories)
	{
		std::shared_ptr<TerminalCategoryDama> category = category_it.second;

		// we can compute CRA per carriers group because a terminal
		// is assigned to one on each frame, depending on its DRA
		for (auto &&carriers: category->getCarriersGroups())
		{
			rate_kbps_t cra_request_kbps = 0;
			rate_kbps_t cra_alloc_kbps = 0;

			this->computeDamaCraPerCarrier(carriers,
			                               category,
				                           cra_request_kbps,
				                           cra_alloc_kbps);
			gw_cra_request_kbps += cra_request_kbps;
			this->gw_cra_alloc_kbps += cra_alloc_kbps;;

			if(cra_alloc_kbps < cra_request_kbps)
			{
				stat = false;
			}
		}
	}
	//this->probe_gw_cra_request->put(this->gw_cra_request_kbps);

	return stat;
}

bool DamaCtrlRcs2Legacy::computeTerminalsRbdcAllocation()
{
	rate_kbps_t gw_rbdc_request_kbps = 0;
	rate_kbps_t gw_rbdc_alloc_kbps = 0;

	for (auto &&category_it: this->categories)
	{
		std::shared_ptr<TerminalCategoryDama> category = category_it.second;
		// we ca compute RBDC per carriers group because a terminal
		// is assigned to one on each frame, depending on its DRA
		for (auto &&carriers: category->getCarriersGroups())
		{
			rate_kbps_t rbdc_request_kbps = 0;
			rate_kbps_t rbdc_alloc_kbps = 0;

			this->computeDamaRbdcPerCarrier(carriers,
			                                category,
			                                rbdc_request_kbps,
			                                rbdc_alloc_kbps);
			gw_rbdc_request_kbps += rbdc_request_kbps;
			gw_rbdc_alloc_kbps += rbdc_alloc_kbps;
		}
	}
	// Output stats and probes
	this->probe_gw_rbdc_req_num->put(gw_rbdc_req_num);
	this->gw_rbdc_req_num = 0;
	this->probe_gw_rbdc_req_size->put(gw_rbdc_request_kbps);
	this->probe_gw_rbdc_alloc->put(gw_rbdc_alloc_kbps);

	return true;
}


bool DamaCtrlRcs2Legacy::computeTerminalsVbdcAllocation()
{
	vol_kb_t gw_vbdc_request_kb = 0;
	vol_kb_t gw_vbdc_alloc_kb = 0;

	for (auto &&category_it: this->categories)
	{
		std::shared_ptr<TerminalCategoryDama> category = category_it.second;
		for (auto &&carriers: category->getCarriersGroups())
		{
			vol_kb_t vbdc_request_kb = 0;
			vol_kb_t vbdc_alloc_kb = 0;

			this->computeDamaVbdcPerCarrier(carriers,
			                                category,
			                                vbdc_request_kb,
			                                vbdc_alloc_kb);
			gw_vbdc_request_kb += vbdc_request_kb;
			gw_vbdc_alloc_kb += vbdc_alloc_kb;
		}
	}

	// Output stats and probes
	this->probe_gw_vbdc_req_num->put(this->gw_vbdc_req_num);
	this->gw_vbdc_req_num = 0;
	this->probe_gw_vbdc_req_size->put(gw_vbdc_request_kb);
	this->probe_gw_vbdc_alloc->put(gw_vbdc_alloc_kb);

	return true;
}


bool DamaCtrlRcs2Legacy::computeTerminalsFcaAllocation()
{
	rate_kbps_t gw_fca_alloc_kbps = 0;

	if(this->fca_kbps == 0)
	{
		LOG(this->log_run_dama, LEVEL_INFO,
		    "SF#%u: no fca, skip\n", this->current_superframe_sf);
		return true;
	}

	for (auto &&category_it: this->categories)
	{
		std::shared_ptr<TerminalCategoryDama> category = category_it.second;
		for (auto &&carriers: category->getCarriersGroups())
		{
			rate_kbps_t fca_alloc_kbps = 0;

			this->computeDamaFcaPerCarrier(carriers,
			                               category,
			                               fca_alloc_kbps);
			gw_fca_alloc_kbps += fca_alloc_kbps;
		}
	}

	// Be careful to use probes only if FCA is enabled
	// Output probes and stats
	this->probe_gw_fca_alloc->put(gw_fca_alloc_kbps);

	return true;
}

void DamaCtrlRcs2Legacy::computeDamaCraPerCarrier(CarriersGroupDama &carriers,
                                                  std::shared_ptr<const TerminalCategoryDama> category,
                                                  rate_kbps_t &request_rate_kbps,
                                                  rate_kbps_t &alloc_rate_kbps)
{
	std::ostringstream buf;
	std::string label = category->getLabel();
	std::string debug;

	unsigned int carrier_id = carriers.getCarriersId();
	rate_pktpf_t remaining_capacity_pktpf;
	rate_pktpf_t total_capacity_pktpf;
	rate_kbps_t simu_cra_kbps = 0;

	buf << "SF#" << this->current_superframe_sf << " carrier "
	    << carrier_id << ", category " << label << ":";
	debug = buf.str();

	// Get the remaining capacity in timeslot number (per frame)
	remaining_capacity_pktpf = carriers.getRemainingCapacity();
	total_capacity_pktpf = this->converter->symToPkt(carriers.getTotalCapacity());

	LOG(this->log_run_dama, LEVEL_INFO,
	    "%s remaining capacity = %u packets per superframe before CRA allocation (total: %u packets)\n",
	    debug.c_str(), remaining_capacity_pktpf, total_capacity_pktpf);

	auto tal = category->getTerminalsInCarriersGroup<TerminalContextDamaRcs>(carrier_id);

	// get total CRA allocation
	for (auto &&terminal: tal)
	{
		FmtDefinition *fmt_def;
		rate_pktpf_t cra_pktpf;
		rate_kbps_t cra_kbps;

		tal_id_t tal_id = terminal->getTerminalId();
		fmt_def = terminal->getFmt();
		if(fmt_def == nullptr)
		{
			continue;
		}
		this->converter->setModulationEfficiency(fmt_def->getModulationEfficiency());

		cra_kbps = terminal->getRequiredCra();
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%d: CRA %u kb/s",
		    debug.c_str(), tal_id, cra_kbps);

		request_rate_kbps += cra_kbps;

		cra_kbps = fmt_def->addFec(cra_kbps);
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%d: CRA with FEC %u kb/s",
		    debug.c_str(), tal_id, cra_kbps);

		cra_pktpf = this->converter->kbpsToPktpf(cra_kbps);
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%d: CRA %u packets per frame",
		    debug.c_str(), tal_id, cra_pktpf);

		// Evaluate the real requested rate (multiple of the timeslot rate)
		cra_kbps = this->converter->pktpfToKbps(cra_pktpf);
		cra_kbps = fmt_def->removeFec(cra_kbps);
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%d: Updated CRA %u kb/s to timeslot use consequence",
		    debug.c_str(), tal_id, cra_kbps);

		if(remaining_capacity_pktpf < cra_pktpf)
		{
			LOG(this->log_run_dama, LEVEL_ERROR,
			    "%s ST%d: Cannot allocate CRA %u packets per superframe (%u kb/s)\n",
			    debug.c_str(), tal_id, cra_pktpf, cra_kbps);
			continue;
		}
		remaining_capacity_pktpf -= cra_pktpf;
		alloc_rate_kbps += cra_kbps;
		terminal->setCraAllocation(cra_kbps);

		// Output probes and stats
		if(tal_id > BROADCAST_TAL_ID)
		{
			simu_cra_kbps += cra_kbps;
		}
		else
		{
			this->probes_st_cra_alloc[tal_id]->put(cra_kbps);
		}
	}

	if(this->simulated)
	{
		this->probes_st_cra_alloc[0]->put(simu_cra_kbps);
	}

	LOG(this->log_run_dama, LEVEL_INFO,
	    "%s remaining capacity = %u packets per superframe after CRA allocation (total: %u packets)\n",
	    debug.c_str(), remaining_capacity_pktpf, total_capacity_pktpf);

	carriers.setRemainingCapacity(remaining_capacity_pktpf);
}

void DamaCtrlRcs2Legacy::computeDamaRbdcPerCarrier(CarriersGroupDama &carriers,
                                                   std::shared_ptr<const TerminalCategoryDama> category,
                                                   rate_kbps_t &request_rate_kbps,
                                                   rate_kbps_t &alloc_rate_kbps)
{
	rate_pktpf_t total_request_pktpf = 0;
	rate_pktpf_t request_pktpf;
	rate_kbps_t request_kbps;
	rate_kbps_t rbdc_alloc_kbps;
	double fair_share;
	rate_pktpf_t rbdc_alloc_pktpf = 0;
	unsigned int carrier_id = carriers.getCarriersId();
	rate_pktpf_t remaining_capacity_pktpf;
	rate_pktpf_t total_capacity_pktpf;
	int simu_rbdc = 0;
	std::ostringstream buf;
	std::string label = category->getLabel();
	std::string debug;

	std::map<tal_id_t, rate_pktpf_t> tal_request_pktpf;

	// set default values
	request_rate_kbps = 0;
	alloc_rate_kbps = 0;

	buf << "SF#" << this->current_superframe_sf << " carrier "
	    << carrier_id << ", category " << label << ":";
	debug = buf.str();

	// Get the remaining capacity in timeslot number (per frame)
	remaining_capacity_pktpf = carriers.getRemainingCapacity();
	total_capacity_pktpf = this->converter->symToPkt(carriers.getTotalCapacity());

	if(remaining_capacity_pktpf == 0)
	{
		LOG(this->log_run_dama, LEVEL_INFO,
		    "%s skipping RBDC allocation: Not enough "
		    "capacity\n", debug.c_str());
		return;
	}

	LOG(this->log_run_dama, LEVEL_INFO,
	    "%s remaining capacity = %u packets per superframe before RBDC allocation (total: %u packets)\n",
	    debug.c_str(), remaining_capacity_pktpf, total_capacity_pktpf);

	auto tal = category->getTerminalsInCarriersGroup<TerminalContextDamaRcs>(carrier_id);

	// get total RBDC requests
	for (auto &&terminal: tal)
	{
		tal_id_t tal_id = terminal->getTerminalId();
		FmtDefinition *fmt_def = terminal->getFmt();
		if(fmt_def == nullptr)
		{
			continue;
		}
		this->converter->setModulationEfficiency(fmt_def->getModulationEfficiency());

		request_kbps = terminal->getRequiredRbdc();
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%d: RBDC request %u kb/s",
		    debug.c_str(), tal_id, request_kbps);

		request_kbps = fmt_def->addFec(request_kbps);
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%d: RBDC request with FEC %u kb/s",
		    debug.c_str(), tal_id, request_kbps);

		request_pktpf = this->converter->kbpsToPktpf(request_kbps);
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%d: RBDC request %u packets per frame",
		    debug.c_str(), tal_id, request_pktpf);
		tal_request_pktpf[tal_id] = request_pktpf;

		// Evaluate the real requested rate (multiple of the timeslot rate)
		request_kbps = this->converter->pktpfToKbps(request_pktpf);
		request_kbps = fmt_def->removeFec(request_kbps);
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%d: Updated RBDC request %u kb/s to timeslot use consequence",
		    debug.c_str(), tal_id, request_kbps);

		total_request_pktpf += request_pktpf;

		// Output stats and probes
		if (request_pktpf > 0)
			this->gw_rbdc_req_num++;

		// Output stats and probes
		request_rate_kbps += request_kbps;
	}

	if(total_request_pktpf == 0)
	{
		LOG(this->log_run_dama, LEVEL_INFO,
		    "%s no RBDC request for this frame.\n", debug.c_str());

		// Output stats and probes
		for (auto &&terminal: tal)
		{
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
	}

	LOG(this->log_run_dama, LEVEL_INFO,
	    "%s: sum of all RBDC requests = %u packets per superframe, "
	    "fair share=%f\n", debug.c_str(),
	    total_request_pktpf, fair_share);

	// first step : serve the integer part of the fair RBDC
	alloc_rate_kbps = 0;
	for (auto &&terminal: tal)
	{
		rate_symps_t rbdc_alloc_symps;
		double fair_rbdc_pktpf;

		tal_id_t tal_id = terminal->getTerminalId();
		FmtDefinition *fmt_def = terminal->getFmt();
		if(fmt_def == nullptr)
		{
			if(tal_id <= BROADCAST_TAL_ID)
			{
				this->probes_st_rbdc_alloc[tal_id]->put(0);
			}
			continue;
		}
		this->converter->setModulationEfficiency(fmt_def->getModulationEfficiency());

		// apply the fair share coef to all requests
		request_pktpf = tal_request_pktpf[tal_id];
		fair_rbdc_pktpf = (double) (request_pktpf / fair_share);

		// take the integer part of fair RBDC
		rbdc_alloc_pktpf = floor(fair_rbdc_pktpf);
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%d: RBDC allocation %u packets per frame",
		    debug.c_str(), tal_id, rbdc_alloc_pktpf);

		rbdc_alloc_kbps = this->converter->pktpfToKbps(rbdc_alloc_pktpf);
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%d: RBDC allocation with FEC %u kb/s",
		    debug.c_str(), tal_id, rbdc_alloc_kbps);

		rbdc_alloc_kbps = fmt_def->removeFec(rbdc_alloc_kbps);
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%d: RBDC allocation %u kb/s",
		    debug.c_str(), tal_id, rbdc_alloc_kbps);

		terminal->setRbdcAllocation(rbdc_alloc_kbps);
		alloc_rate_kbps += rbdc_alloc_kbps;

		// decrease the total capacity
		remaining_capacity_pktpf -= rbdc_alloc_pktpf;

		// Output probes and stats
		if(tal_id > BROADCAST_TAL_ID)
		{
			simu_rbdc += rbdc_alloc_kbps;
		}
		else
		{
			this->probes_st_rbdc_alloc[tal_id]->put(rbdc_alloc_kbps);
		}
		rbdc_alloc_symps = this->converter->pktpfToSymps(rbdc_alloc_pktpf);
		this->carrier_return_remaining_capacity[label][carrier_id] -= rbdc_alloc_symps;
		this->category_return_remaining_capacity[label] -= rbdc_alloc_symps;
		this->gw_remaining_capacity -= rbdc_alloc_symps;

		if(fair_share > 1.0)
		{
			// add the decimal part of the fair RBDC
			double rbdc_credit_kbps = (fair_rbdc_pktpf - rbdc_alloc_pktpf)
				* this->converter->getPacketBitLength()
				/ (double)(this->converter->getFrameDuration().count());
			rbdc_credit_kbps /= (fmt_def->getCodingRate());
			terminal->addRbdcCredit(rbdc_credit_kbps);

			LOG(this->log_run_dama, LEVEL_DEBUG,
				"%s ST%u: RBDC credit %u kb/s\n",
				debug.c_str(), tal_id, rbdc_credit_kbps);
		}
	}
	if(this->simulated)
	{
		this->probes_st_rbdc_alloc[0]->put(simu_rbdc);
	}

	// second step : RBDC decimal part treatment
	if(fair_share > 1.0)
	{
		// sort terminal according to their remaining credit
		std::stable_sort(tal.begin(), tal.end(),
		                 TerminalContextDamaRcs::sortByRemainingCredit);
		for (auto &&terminal: tal)
		{
			if (remaining_capacity_pktpf <= 0)
			{
				break;
			}
			rate_kbps_t slot_kbps;
			double credit_kbps;

			tal_id_t tal_id = terminal->getTerminalId();
			FmtDefinition *fmt_def = terminal->getFmt();
			if(fmt_def == nullptr)
			{
				continue;
			}
			this->converter->setModulationEfficiency(fmt_def->getModulationEfficiency());

			slot_kbps = fmt_def->removeFec(this->converter->pktpfToKbps(1));
			credit_kbps = terminal->getRbdcCredit();
			LOG(this->log_run_dama, LEVEL_DEBUG,
			    "%s step 2 scanning ST%u remaining capacity=%u packet "
			    "credit=%f packet\n", debug.c_str(),
			    tal_id, remaining_capacity_pktpf,
			    credit_kbps / slot_kbps);
			if(credit_kbps > slot_kbps)
			{
				rate_kbps_t cra_kbps;
				rate_kbps_t max_rbdc_kbps;

				max_rbdc_kbps = terminal->getMaxRbdc();
				cra_kbps = terminal->getCraAllocation();
				rbdc_alloc_kbps = terminal->getRbdcAllocation();

				if(max_rbdc_kbps - rbdc_alloc_kbps - cra_kbps > slot_kbps)
				{
					rate_symps_t slot_symps;
					// enough capacity to allocate
					terminal->setRbdcAllocation(rbdc_alloc_kbps + slot_kbps);
					terminal->addRbdcCredit(-slot_kbps);
					alloc_rate_kbps += slot_kbps;
					remaining_capacity_pktpf--;
					LOG(this->log_run_dama, LEVEL_DEBUG,
					    "%s step 2 allocating 1 timeslot to ST%u\n",
					    debug.c_str(), tal_id);
					// Update probes and stats
					slot_symps = this->converter->pktpfToSymps(1);
					this->carrier_return_remaining_capacity[label][carrier_id] -= slot_symps;
					this->category_return_remaining_capacity[label] -= slot_symps;
					this->gw_remaining_capacity -= slot_symps;
				}
			}
		}
	}

	LOG(this->log_run_dama, LEVEL_INFO,
	    "%s remaining capacity = %u packets per superframe after RBDC allocation (total: %u packets)\n",
	    debug.c_str(), remaining_capacity_pktpf, total_capacity_pktpf);

	carriers.setRemainingCapacity(remaining_capacity_pktpf);
}

void DamaCtrlRcs2Legacy::computeDamaVbdcPerCarrier(CarriersGroupDama &carriers,
                                                   std::shared_ptr<const TerminalCategoryDama> category,
                                                   vol_kb_t &request_vol_kb,
                                                   vol_kb_t &alloc_vol_kb)
{
	unsigned int carrier_id = carriers.getCarriersId();
	rate_pktpf_t remaining_capacity_pktpf;
	rate_pktpf_t total_capacity_pktpf;
	int simu_vbdc = 0;
	std::ostringstream buf;
	std::string label = category->getLabel();
	std::string debug;

	request_vol_kb = 0;
	alloc_vol_kb = 0;

	buf << "SF#" << this->current_superframe_sf << " carrier "
	    << carrier_id << ", category " << label << ":";
	debug = buf.str();

	// Get the remaining capacity in timeslot number (per frame)
	remaining_capacity_pktpf = carriers.getRemainingCapacity();
	total_capacity_pktpf = this->converter->symToPkt(carriers.getTotalCapacity());

	auto tal = category->getTerminalsInCarriersGroup<TerminalContextDamaRcs>(carrier_id);
	if(remaining_capacity_pktpf == 0)
	{
		LOG(this->log_run_dama, LEVEL_NOTICE,
		    "%s skipping VBDC dama computation: Not enough "
		    "capacity\n", debug.c_str());

		// Output stats and probes
		for (auto &&terminal: tal)
		{
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
	    "%s remaining capacity = %u packets per superframe before VBDC allocation (total: %u packets)\n",
	    debug.c_str(), remaining_capacity_pktpf, total_capacity_pktpf);

	if(tal.empty())
	{
		// no ST
		return;
	}

	// try to serve the required VBDC
	// the setVbdcAllocation functions had updated the VBDC requests
	// sort terminal according to their new VBDC requests
	decltype(tal)::iterator tal_it;
	std::stable_sort(tal.begin(), tal.end(),
	                 TerminalContextDamaRcs::sortByVbdcReq);
	for(tal_it = tal.begin(); tal_it != tal.end() && 0 < remaining_capacity_pktpf; ++tal_it)
	{
		vol_kb_t request_kb;
		vol_pkt_t request_pkt;
		vol_kb_t alloc_kb;
		vol_pkt_t alloc_pkt;
		rate_symps_t alloc_symps;
		FmtDefinition *fmt_def;

		auto terminal = *tal_it;
		tal_id_t tal_id = terminal->getTerminalId();
		fmt_def = terminal->getFmt();
		if(fmt_def == nullptr)
		{
			// Output probes and stats
			if(tal_id <= BROADCAST_TAL_ID)
			{
				this->probes_st_vbdc_alloc[tal_id]->put(0);
			}
			continue;
		}
		this->converter->setModulationEfficiency(fmt_def->getModulationEfficiency());

		request_kb = terminal->getRequiredVbdc();
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%u: VBDC request %u kb",
		    debug.c_str(), tal_id, request_kb);

		request_kb = fmt_def->addFec(request_kb);
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%u: VBDC request with FEC %u kb",
		    debug.c_str(), tal_id, request_kb);

		request_pkt = this->converter->kbitsToPkt(request_kb);
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%u: VBDC request %u packets",
		    debug.c_str(), tal_id, request_pkt);

		if(request_pkt <= 0)
		{
			continue;
		}
		this->gw_vbdc_req_num++;
		request_vol_kb += request_kb;

		if(request_pkt <= remaining_capacity_pktpf)
		{
			// enough capacity to allocate
			alloc_pkt = request_pkt;
		}
		else
		{
			// not enough capacity to allocate
			alloc_pkt = remaining_capacity_pktpf;
		}
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%u: VBDC allocation %u packets",
		    debug.c_str(), tal_id, alloc_pkt);
		remaining_capacity_pktpf -= alloc_pkt;

		alloc_kb = this->converter->pktToKbits(alloc_pkt);
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%u: VBDC allocation with FEC %u kb",
		    debug.c_str(), tal_id, alloc_kb);

		alloc_kb = fmt_def->removeFec(alloc_kb);
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%u: VBDC allocation %u kb",
		    debug.c_str(), tal_id, alloc_kb);

		terminal->setVbdcAllocation(alloc_kb);
		alloc_vol_kb += alloc_kb;

		// Output probes and stats
		if(tal_id > BROADCAST_TAL_ID)
		{
			simu_vbdc += alloc_kb;
		}
		else
		{
			this->probes_st_vbdc_alloc[tal_id]->put(alloc_kb);
		}
		alloc_symps = this->converter->pktpfToSymps(alloc_pkt);
		this->carrier_return_remaining_capacity[label][carrier_id] -= alloc_symps;
		this->category_return_remaining_capacity[label] -= alloc_symps;
		this->gw_remaining_capacity -= alloc_symps;
	}

	if(this->simulated)
	{
		this->probes_st_vbdc_alloc[0]->put(simu_vbdc);
	}

	// Check if other terminals required capacity
	for(; tal_it != tal.end(); ++tal_it)
	{
		vol_kb_t request_kb;
		auto terminal = *tal_it;
		request_kb = terminal->getRequiredVbdc();
		if(request_kb > 0)
		{
			request_vol_kb += request_kb;
			this->gw_vbdc_req_num++;
		}
	}

	LOG(this->log_run_dama, LEVEL_INFO,
	    "%s remaining capacity = %u packets per superframe after VBDC allocation (total: %u packets)\n",
	    debug.c_str(), remaining_capacity_pktpf, total_capacity_pktpf);

	carriers.setRemainingCapacity(remaining_capacity_pktpf);
}

// TODO it would be better if, at the end of allocations computation,
//      we try to move some terminals not totally served in supported carriers
//      (in the same category and with supported MODCOD value) in which there
//      is still capacity
void DamaCtrlRcs2Legacy::computeDamaFcaPerCarrier(CarriersGroupDama &carriers,
                                                  std::shared_ptr<const TerminalCategoryDama> category,
                                                  rate_kbps_t &alloc_rate_kbps)
{
	unsigned int carrier_id = carriers.getCarriersId();
	rate_pktpf_t remaining_capacity_pktpf;
	rate_pktpf_t total_capacity_pktpf;
	rate_pktpf_t fca_pktpf;
	int simu_fca = 0;
	std::ostringstream buf;
	std::string label = category->getLabel();
	std::string debug;

	alloc_rate_kbps = 0;

	buf << "SF#" << this->current_superframe_sf << " carrier "
	    << carrier_id << ", category " << label << ":";
	debug = buf.str();

	auto tal = category->getTerminalsInCarriersGroup<TerminalContextDamaRcs>(carrier_id);
	auto tal_it = tal.begin();
	if(tal_it == tal.end())
	{
		// no ST
		return;
	}

	remaining_capacity_pktpf = carriers.getRemainingCapacity();
	total_capacity_pktpf = this->converter->symToPkt(carriers.getTotalCapacity());

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
	    "%s remaining capacity = %u packets per superframe before FCA allocation (total: %u packets)\n",
	    debug.c_str(), remaining_capacity_pktpf, total_capacity_pktpf);

	// sort terminal according to their remaining credit
	// this is a random but logical choice
	std::stable_sort(tal.begin(), tal.end(),
	                 TerminalContextDamaRcs::sortByRemainingCredit);

	while(tal_it != tal.end() && 0 < remaining_capacity_pktpf)
	{
		rate_pktpf_t fca_alloc_pktpf;
		rate_kbps_t fca_alloc_kbps;
		auto terminal = *tal_it;
		tal_id_t tal_id = terminal->getTerminalId();
		FmtDefinition *fmt_def = terminal->getFmt();
		if(fmt_def == nullptr)
		{
			continue;
		}

		fca_pktpf = this->converter->kbpsToPktpf(fmt_def->addFec(this->fca_kbps));
		if (remaining_capacity_pktpf > fca_pktpf)
		{
			fca_alloc_pktpf = fca_pktpf;
			remaining_capacity_pktpf -= fca_pktpf;
		}
		else
		{
			fca_alloc_pktpf = remaining_capacity_pktpf;
			remaining_capacity_pktpf = 0;
		}
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%u: FCA alloc %u packets per superframe",
		    debug.c_str(), tal_id, fca_alloc_pktpf);

		fca_alloc_kbps = fmt_def->removeFec(this->converter->pktpfToKbps(fca_alloc_pktpf));
		LOG(this->log_run_dama, LEVEL_DEBUG,
		    "%s ST%u: FCA alloc %u kb/s",
		    debug.c_str(), tal_id, fca_alloc_kbps);
		terminal->setFcaAllocation(fca_alloc_kbps);
		alloc_rate_kbps += fca_alloc_kbps;

		// Output probes and stats
		if(tal_id > BROADCAST_TAL_ID)
		{
			simu_fca += fca_alloc_kbps;
		}
		else
		{
			this->probes_st_fca_alloc[tal_id]->put(fca_alloc_kbps);
		}
		this->carrier_return_remaining_capacity[label][carrier_id] -= fca_alloc_kbps;
		this->category_return_remaining_capacity[label] -= fca_alloc_kbps;
		this->gw_remaining_capacity -= fca_alloc_kbps;

		tal_it++;
	}
	if(this->simulated)
	{
		this->probes_st_fca_alloc[0]->put(simu_fca);
	}

	LOG(this->log_run_dama, LEVEL_INFO,
	    "%s remaining capacity = %u packets per superframe after FCA allocation (total: %u packets)\n",
	    debug.c_str(), remaining_capacity_pktpf, total_capacity_pktpf);

	carriers.setRemainingCapacity(remaining_capacity_pktpf);
}
