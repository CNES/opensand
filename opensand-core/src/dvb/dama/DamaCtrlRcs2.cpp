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
#include "OpenSandModelConf.h"
#include "TerminalContextDamaRcs.h"
#include "CarriersGroupDama.h"
#include "UnitConverterFixedSymbolLength.h"

#include <opensand_output/Output.h>

#include <math.h>
#include <vector>


/**
 * Constructor
 */
DamaCtrlRcs2::DamaCtrlRcs2(spot_id_t spot):
	DamaCtrl(spot),
	converter(nullptr)
{
}


bool DamaCtrlRcs2::init()
{
	vol_sym_t length_sym = 0;

	// Ensure parent init has been done
	if(!this->is_parent_init)
	{
		LOG(this->log_init, LEVEL_ERROR, 
		    "Parent 'init()' method must be called first.\n");
		return false;
	}
	
	if(!OpenSandModelConf::Get()->getRcs2BurstLength(length_sym))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get RCS2 burst length value");
		return false;
	}
	if(length_sym == 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "invalid value '%u' value of RCS2 burst length", length_sym);
		return false;
	}
	LOG(this->log_init, LEVEL_INFO,
	    "Burst length = %u sym\n", length_sym);
	
	try
	{
		this->converter = std::make_unique<UnitConverterFixedSymbolLength>(this->frame_duration, 0, length_sym);
	}
	catch (const std::bad_alloc&)
	{
		LOG(this->log_init, LEVEL_ERROR, 
		    "Unit converter generation failed.\n");
		return false;
	}
	
	return true;
}

bool DamaCtrlRcs2::hereIsSAC(Rt::Ptr<Sac> sac)
{
	vol_kb_t request_kb;
	rate_kbps_t request_kbps;
	tal_id_t tal_id = sac->getTerminalId();
	std::vector<cr_info_t> requests = sac->getRequests();

	// Checking if the station is registered
	// if we get GW terminal ID this is for physical layer parameters
	auto terminal = std::dynamic_pointer_cast<TerminalContextDamaRcs>(this->getTerminalContext(tal_id));
	if(!terminal && !OpenSandModelConf::Get()->isGw(tal_id))
	{
		LOG(this->log_sac, LEVEL_ERROR, 
		    "SF#%u: CR for an unknown st (logon_id=%u). "
		    "Discarded.\n" , this->current_superframe_sf, tal_id);
		return false;
	}

	for (auto&& cr_info: requests)
	{
		// take into account the new request
		switch(cr_info.type)
		{
			case ReturnAccessType::dama_vbdc:
				request_kb = cr_info.value;
				LOG(this->log_sac, LEVEL_INFO,
				    "SF#%u: ST%u received VBDC requests %u kb\n",
				    this->current_superframe_sf, tal_id, request_kb);
				
				request_kb = std::min(request_kb, terminal->getMaxVbdc());
				LOG(this->log_sac, LEVEL_INFO,
				    "SF#%u: ST%u updated VBDC requests %u kb (<= max VBDC %u kb)\n",
				    this->current_superframe_sf, tal_id, request_kb, terminal->getMaxVbdc());

				terminal->setRequiredVbdc(request_kb);
				this->enable_vbdc = true;

				if(tal_id > BROADCAST_TAL_ID)
				{
					this->record_event("CR st", tal_id,
					                   " cr=", request_kb,
					                   " type=", to_underlying(cr_info.type));
				}
				break;

			case ReturnAccessType::dama_rbdc:
				request_kbps = cr_info.value;
				LOG(this->log_sac, LEVEL_INFO,
				    "SF#%u: ST%u received RBDC requests %u kb/s\n",
				    this->current_superframe_sf, tal_id, request_kbps);

				// remove the CRA of the RBDC request
				// the CRA is not taken into acount on ST side
				request_kbps = std::max(request_kbps - terminal->getRequiredCra(), 0);
				LOG(this->log_sac, LEVEL_INFO,
				    "SF#%u: ST%u updated RBDC requests %u kb/s (removing CRA %u kb/s)\n",
				    this->current_superframe_sf, tal_id, request_kbps, terminal->getRequiredCra());

				request_kbps = std::min(request_kbps, terminal->getMaxRbdc());
				LOG(this->log_sac, LEVEL_INFO,
				    "SF#%u: ST%u updated RBDC requests %u kb/s (<= max RBDC %u kb/s)\n",
				    this->current_superframe_sf, tal_id, request_kbps, terminal->getMaxRbdc());

				terminal->setRequiredRbdc(request_kbps);
				this->enable_rbdc = true;
				if(tal_id > BROADCAST_TAL_ID)
				{
					this->record_event("CR st", tal_id,
					                   " cr=", request_kb,
					                   " type=", to_underlying(cr_info.type));
				}
				break;

			default:
				LOG(this->log_sac, LEVEL_INFO,
				    "SF#%u: ST%u received request of unkwon type %d\n",
				    this->current_superframe_sf, tal_id, cr_info.type);
				break;
		}
	}

	return true;
}

bool DamaCtrlRcs2::buildTTP(Ttp &ttp)
{
	for (auto &&[label, category]: this->categories)
	{
		const std::vector<std::shared_ptr<TerminalContext>> &terminals = category->getTerminals();
		
		LOG(this->log_ttp, LEVEL_DEBUG,
		    "SF#%u: Category %s has %zu terminals\n",
		    this->current_superframe_sf,
		    label, terminals.size());
		for (auto &&term: terminals)
		{
			std::shared_ptr<TerminalContextDamaRcs> terminal = std::dynamic_pointer_cast<TerminalContextDamaRcs>(term);
			vol_kb_t total_allocation_kb = 0;

			// we need to do that else some CRA will be allocated and the terminal
			// will send data even if there is no MODCOD robust enough
			if(terminal->getFmtId() != 0)
			{
				LOG(this->log_ttp, LEVEL_DEBUG,
				    "[Tal %u] total volume = %u kb ; total rate = %u kb/s (%u kb for one frame)",
				    terminal->getTerminalId(),
				    terminal->getTotalVolumeAllocation(),
				    terminal->getTotalRateAllocation(),
				    this->converter->psToPf(terminal->getTotalRateAllocation()));
				total_allocation_kb += terminal->getTotalVolumeAllocation();
				total_allocation_kb += this->converter->psToPf(terminal->getTotalRateAllocation());
			}
			LOG(this->log_ttp, LEVEL_DEBUG,
			    "[Tal %u] total allocation = %u kb",
			    terminal->getTerminalId(),
			    total_allocation_kb);

			//FIXME: is the offset to be 0 ???
			if(!ttp.addTimePlan(0 /*FIXME: should it be the frame_counter of the bloc_ncc ?*/,
			                    terminal->getTerminalId(),
			                    0,
			                    total_allocation_kb,
			                    terminal->getFmtId(),
			                    0))
			{
				LOG(this->log_ttp, LEVEL_ERROR,
				    "SF#%u: cannot add TimePlan for terminal %u\n",
				    this->current_superframe_sf, terminal->getTerminalId());
				continue;
			}
		}
	}
	ttp.build();

	return true;
}

bool DamaCtrlRcs2::applyPepCommand(std::unique_ptr<PepRequest> request)
{
	rate_kbps_t cra_kbps;
	rate_kbps_t max_rbdc_kbps;
	rate_kbps_t rbdc_kbps;

	// check that the ST is logged on
	auto terminal = std::dynamic_pointer_cast<TerminalContextDamaRcs>(this->getTerminalContext(request->getStId()));
	if(terminal == nullptr)
	{
		LOG(this->log_pep, LEVEL_ERROR, 
		    "SF#%u: ST%d is not logged on, ignore %s request\n",
		    this->current_superframe_sf, request->getStId(),
		    request->getType() == PEP_REQUEST_ALLOCATION ?
		    "allocation" : "release");
		return false;
	}

	// update CRA allocation ?
	cra_kbps = request->getCra();
	if(cra_kbps != 0)
	{
		terminal->setRequiredCra(cra_kbps);
		LOG(this->log_pep, LEVEL_NOTICE,
		    "SF#%u: ST%u: update the CRA value to %u kbits/s\n",
		    this->current_superframe_sf,
		    request->getStId(), request->getCra());
	}

	// update RDBCmax threshold ?
	max_rbdc_kbps = request->getRbdcMax();
	if(max_rbdc_kbps != 0)
	{
		// Output probes and stats
		this->gw_rbdc_max_kbps -= terminal->getMaxRbdc();

		terminal->setMaxRbdc(max_rbdc_kbps);
		LOG(this->log_pep, LEVEL_NOTICE,
		    "SF#%u: ST%u: update RBDC max to %u kbits/s\n",
		    this->current_superframe_sf,
		    request->getStId(), request->getRbdcMax());

		// Output probes and stats
		this->gw_rbdc_max_kbps += max_rbdc_kbps;
		this->probe_gw_rbdc_max->put(this->gw_rbdc_max_kbps);
		this->probes_st_rbdc_max[terminal->getTerminalId()]->put(max_rbdc_kbps);
	}

	// inject one RDBC allocation ?
	rbdc_kbps = request->getRbdc();
	if(rbdc_kbps != 0)
	{
		// increase the RDBC timeout in order to be sure that RDBC
		// will not expire before the session is established
		terminal->updateRbdcTimeout(100);

		terminal->setRequiredRbdc(rbdc_kbps);
		LOG(this->log_pep, LEVEL_NOTICE,
		    "SF#%u: ST%u: inject RDBC request of %u kbits/s\n",
		    this->current_superframe_sf,
		    request->getStId(), request->getRbdc());

		// change back RDBC timeout
		terminal->updateRbdcTimeout(this->rbdc_timeout_sf);
	}

	return true;
}

void DamaCtrlRcs2::updateRequiredFmts()
{
	tal_id_t tal_id;
	double cni;
	fmt_id_t fmt_id;
	
	if(!this->simulated)
	{
		// Update required Fmt in function of the Cni
		for (auto &&terminal_it: this->terminals)
		{
			auto terminal = std::dynamic_pointer_cast<TerminalContextDamaRcs>(terminal_it.second);
			tal_id = terminal->getTerminalId();

			// Get CNI
			cni = this->input_sts->getRequiredCni(tal_id);
			LOG(this->log_fmt, LEVEL_DEBUG,
			    "SF#%u: ST%u CNI before affectation: %f\n",
			    this->current_superframe_sf, tal_id, cni);
			
			// Get required Modcod from the CNI
			fmt_id = this->input_modcod_def->getRequiredModcod(cni);
			if(fmt_id == 0)
			{
				fmt_id = this->input_modcod_def->getMinId();
			}
			LOG(this->log_fmt, LEVEL_DEBUG,
				"SF#%u: ST%u FMT ID before affectation (CNI %f): %u\n",
				this->current_superframe_sf, tal_id, cni, fmt_id);
	
			// Set required Modcod to the terminal context
			terminal->setRequiredFmt(&(this->input_modcod_def->getDefinition(fmt_id)));
		}
	}
	else
	{
		// Update required Fmt in function of the simulation file
		for (auto &&terminal_it: this->terminals)
		{
			auto terminal = std::dynamic_pointer_cast<TerminalContextDamaRcs>(terminal_it.second);
			tal_id = terminal->getTerminalId();

			// Get required Modcod from the simulation file
			fmt_id = this->input_sts->getCurrentModcodId(tal_id);
			if(fmt_id == 0)
			{
				fmt_id = this->input_modcod_def->getMinId();
			}
			LOG(this->log_fmt, LEVEL_DEBUG,
				"SF#%u: ST%u simulated FMT ID before affectation: %u\n",
				this->current_superframe_sf, tal_id, fmt_id);
	
			// Set required Modcod to the terminal context
			terminal->setRequiredFmt(&(this->input_modcod_def->getDefinition(fmt_id)));
		}
	}
}

bool DamaCtrlRcs2::updateWaveForms()
{
	for (auto &&terminal_it: this->terminals)
	{
		auto terminal = std::dynamic_pointer_cast<TerminalContextDamaRcs>(terminal_it.second);
		tal_id_t tal_id = terminal->getTerminalId();
		unsigned int required_fmt;
		unsigned int available_fmt = 0; // not in the table

		// get the required Fmt for the current terminal
		FmtDefinition *fmt_def = terminal->getRequiredFmt();
		required_fmt = fmt_def != nullptr ? fmt_def->getId() : 0;

		// get the category
		auto category_it = this->categories.find(terminal->getCurrentCategory());
		if(category_it == this->categories.end())
		{
			LOG(this->log_fmt, LEVEL_ERROR,
			    "SF#%u: unable to find category associated with "
			    "terminal %u\n", this->current_superframe_sf, tal_id);
			continue;
		}
		auto &carriers_group = category_it->second->getCarriersGroups();

		// check current carrier has the required FMT
		for (auto &&carriers: carriers_group)
		{
			unsigned int fmt;
			// Find the terminal carrier
			if(carriers.getCarriersId() != terminal->getCarrierId())
			{
				continue;
			}

			// Check the terminal carrier handles the required FMT
			fmt = carriers.getNearestFmtId(required_fmt);
			if(fmt != 0)
			{
				available_fmt = fmt;
				break;
			}
		}

		if(available_fmt == 0)
		{
			// get an available MODCOD id for this terminal among carriers
			for (auto &&carriers: carriers_group)
			{
				unsigned int fmt;
				// FMT groups should only have one FMT id here, so get nearest should
				// return the FMT id of the carrier
				fmt = carriers.getNearestFmtId(required_fmt);
				if(required_fmt <= fmt)
				{
					// we have a carrier with the corresponding MODCOD
					terminal->setCarrierId(carriers.getCarriersId());
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
					terminal->setCarrierId(carriers.getCarriersId());
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
		terminal->setFmt(&(this->input_modcod_def->getDefinition(available_fmt)));
	}
	return true;
}

bool DamaCtrlRcs2::createTerminal(std::shared_ptr<TerminalContextDama> &terminal,
                                  tal_id_t tal_id,
                                  rate_kbps_t cra_kbps,
                                  rate_kbps_t max_rbdc_kbps,
                                  time_sf_t rbdc_timeout_sf,
                                  vol_kb_t max_vbdc_kb)
{
	fmt_id_t fmt_id;

	std::shared_ptr<TerminalContextDamaRcs> term;
	try
	{
		term = std::make_shared<TerminalContextDamaRcs>(tal_id,
	                                                    cra_kbps,
	                                                    max_rbdc_kbps,
	                                                    rbdc_timeout_sf,
	                                                    max_vbdc_kb);
	}
	catch (const std::bad_alloc&)
	{
		LOG(this->log_logon, LEVEL_ERROR,
		    "SF#%u: cannot allocate terminal %u\n",
		    this->current_superframe_sf, tal_id);
		return false;
	}
	terminal = term;

	// Get the best Modcod
	fmt_id = this->input_modcod_def->getMaxId();
	if(fmt_id == 0)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
			"SF#%u: cannot find the best MODCOD id for ST %u\n",
		    this->current_superframe_sf, tal_id);
		return true;
	}
	LOG(this->log_fmt, LEVEL_DEBUG,
		"SF#%u: ST%u FMT ID before affectation (the best FMT): %u\n",
	    this->current_superframe_sf, tal_id, fmt_id);

	// Set required Modcod to the terminal context
	term->setRequiredFmt(&(this->input_modcod_def->getDefinition(fmt_id)));

	return true;
}


bool DamaCtrlRcs2::removeTerminal(std::shared_ptr<TerminalContextDama> &terminal)
{
	terminal = nullptr;
	return true;
}

bool DamaCtrlRcs2::resetTerminalsAllocations()
{
	bool ret = true;
	for(auto it = this->terminals.begin(); it != this->terminals.end(); it++)
	{
		auto terminal = std::dynamic_pointer_cast<TerminalContextDamaRcs>(it->second);
		double credit_kbps = terminal->getRbdcCredit();
		rate_kbps_t request_kbps = terminal->getRequiredRbdc();

		// Reset allocation (in slots)
		terminal->setCraAllocation(0);
		terminal->setRbdcAllocation(0);
		terminal->setVbdcAllocation(0);
		terminal->setFcaAllocation(0);

		// Update timer
		terminal->decrementTimer();
		if(0 < terminal->getTimer() && 0.0 < credit_kbps)
		{
			rate_kbps_t timeslot_kbps;
			FmtDefinition *fmt_def = terminal->getFmt();
			if(fmt_def == NULL)
			{
				terminal->setRbdcCredit(0.0);
				ret = false;
				continue;
			}
			this->converter->setModulationEfficiency(fmt_def->getModulationEfficiency());

			timeslot_kbps = this->converter->pktpfToKbps(1);
			
			// Update RBDC request and credit (in kb/s)
			credit_kbps = std::max(credit_kbps - timeslot_kbps, 0.0);
			request_kbps += timeslot_kbps;

			// Set RBDC request and credit (in kb/s)
			terminal->setRequiredRbdc(request_kbps);
			terminal->setRbdcCredit(credit_kbps);
		}
	}

	return ret;
}

bool DamaCtrlRcs2::resetCarriersCapacity()
{
	rate_symps_t gw_return_total_capacity_symps = 0;

	// Initialize the capacity of carriers
	for (auto &&category_it: this->categories)
	{
		rate_symps_t category_return_capacity_symps = 0;
		std::shared_ptr<TerminalCategoryDama> category = category_it.second;
		std::string label = category->getLabel();

		for (auto &&carriers: category->getCarriersGroups())
		{
			unsigned int carrier_id = carriers.getCarriersId();
			rate_symps_t remaining_capacity_symps;
			rate_pktpf_t remaining_capacity_pktpf;

			// we have several MODCOD for each carrier so we can't convert
			// from bauds to kbits
			remaining_capacity_symps = carriers.getTotalCapacity();
			remaining_capacity_pktpf = this->converter->symToPkt(remaining_capacity_symps);

			// initialize remaining capacity with total capacity in
			// packet per superframe as it is the unit used in DAMA computations
			carriers.setRemainingCapacity(remaining_capacity_pktpf);
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
			auto &probes_capa = this->probes_carrier_return_capacity[label];
			if(probes_capa.find(carrier_id) == probes_capa.end())
			{
				auto probe = this->generateCarrierCapacityProbe(label, carrier_id, "Available");
				probes_capa.emplace(carrier_id, probe);
			}
			if(this->carrier_return_remaining_capacity[label].find(carrier_id)
			   == this->carrier_return_remaining_capacity[label].end())
			{
				this->carrier_return_remaining_capacity[label].emplace(carrier_id, 0);
			}
			probes_capa[carrier_id]->put(remaining_capacity_symps);
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

std::shared_ptr<Probe<int>> DamaCtrlRcs2::generateGwCapacityProbe(std::string name) const
{
	return Output::Get()->registerProbe<int>(output_prefix + "Up/Return total capacity." + name,
	                                         "Sym/s", true, SAMPLE_LAST);
}

std::shared_ptr<Probe<int>> DamaCtrlRcs2::generateCategoryCapacityProbe(std::string category_label,
                                                                        std::string name) const
{
	return Output::Get()->registerProbe<int>(output_prefix + category_label + ".Up/Return capacity.Total." + name,
	                                         "Sym/s", true, SAMPLE_LAST);
}

std::shared_ptr<Probe<int>> DamaCtrlRcs2::generateCarrierCapacityProbe(std::string category_label,
                                                                       unsigned int carrier_id,
                                                                       std::string name) const
{
	return Output::Get()->registerProbe<int>(output_prefix + category_label + ".Up/Return capacity.Carrier" +
	                                         std::to_string(carrier_id) + "." + name,
	                                         "Sym/s", true, SAMPLE_LAST);
}
