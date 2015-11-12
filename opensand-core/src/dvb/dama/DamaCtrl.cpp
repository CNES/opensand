/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
 * @file    DamaCtrl.cpp
 * @brief   This class defines the DAMA controller interfaces
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */


#include "DamaCtrl.h"

#include <opensand_output/Output.h>

#include <assert.h>
#include <math.h>

#include <algorithm>

using std::pair;

DamaCtrl::DamaCtrl(spot_id_t spot):
	is_parent_init(false),
	converter(NULL),
	terminals(), // TODO not very useful, they are stored in categories
	with_phy_layer(false),
	current_superframe_sf(0),
	frame_duration_ms(0),
	rbdc_timeout_sf(0),
	fca_kbps(0),
	enable_rbdc(false),
	enable_vbdc(false),
	available_bandplan_khz(0),
	categories(),
	terminal_affectation(),
	default_category(NULL),
	input_sts(NULL),
	input_modcod_def(NULL),
	roll_off(0.0),
	simulated(false),
	spot_id(spot)
{
	// Output Log
	this->log_init = Output::registerLog(LEVEL_WARNING, "Dvb.init");
	this->log_logon = Output::registerLog(LEVEL_WARNING, "Dvb.DamaCtrl.Logon");
	this->log_super_frame_tick = Output::registerLog(LEVEL_WARNING,
	                                                 "Dvb.DamaCtrl."
	                                                 "SuperFrameTick");
	this->log_run_dama = Output::registerLog(LEVEL_WARNING,
	                                         "Dvb.DamaCtrl.RunDama");
	this->log_sac = Output::registerLog(LEVEL_WARNING, "Dvb.SAC");
	this->log_ttp = Output::registerLog(LEVEL_WARNING, "Dvb.TTP");
	this->log_pep = Output::registerLog(LEVEL_WARNING, "Dvb.Ncc.PEP");
	this->log_fmt = Output::registerLog(LEVEL_WARNING, "Dvb.Fmt.Update");

	// Output probes and stats
	this->probe_gw_rbdc_req_num = NULL;
	this->probe_gw_rbdc_req_size = NULL;
	this->probe_gw_vbdc_req_num = NULL;
	this->probe_gw_vbdc_req_size = NULL;
	this->probe_gw_cra_alloc = NULL;
	this->probe_gw_rbdc_alloc = NULL;
	this->probe_gw_rbdc_max = NULL;
	this->probe_gw_vbdc_alloc = NULL;
	this->probe_gw_fca_alloc = NULL;
	this->probe_gw_return_total_capacity = NULL;
	this->probe_gw_return_remaining_capacity = NULL;
	this->probe_gw_st_num = NULL;
}

DamaCtrl::~DamaCtrl()
{
	TerminalCategories<TerminalCategoryDama>::iterator cat_it;
	if(this->converter)
	{
		delete this->converter;
	}

	for(DamaTerminalList::iterator it = this->terminals.begin();
	    it != this->terminals.end(); ++it)
	{
		delete it->second;
	}
	this->terminals.clear();

	for(cat_it = this->categories.begin();
	    cat_it != this->categories.end(); ++cat_it)
	{
		delete (*cat_it).second;
	}
	this->categories.clear();

	this->terminal_affectation.clear();
}


bool DamaCtrl::initParent(time_ms_t frame_duration_ms,
                          bool with_phy_layer,
                          vol_bytes_t packet_length_bytes,
                          time_sf_t rbdc_timeout_sf,
                          rate_kbps_t fca_kbps,
                          TerminalCategories<TerminalCategoryDama> categories,
                          TerminalMapping<TerminalCategoryDama> terminal_affectation,
                          TerminalCategoryDama *default_category,
                          const ListStFmt *const input_sts,
                          FmtDefinitionTable *const input_modcod_def,
                          bool simulated)
{
	this->frame_duration_ms = frame_duration_ms;
	this->with_phy_layer = with_phy_layer;
	this->rbdc_timeout_sf = rbdc_timeout_sf;
	this->fca_kbps = fca_kbps;
	this->input_sts = input_sts;
	this->input_modcod_def = input_modcod_def;
	this->simulated = simulated;

	this->converter = new UnitConverter(packet_length_bytes,
	                                    this->frame_duration_ms);
	if(this->converter == NULL)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot create the Unit Converter\n");
		goto error;
	}

	this->categories = categories;

	// we keep terminal affectation and default category but these affectations
	// and the default category can concern non DAMA categories
	// so be careful when adding a new terminal
	this->terminal_affectation = terminal_affectation;

	this->default_category = default_category;
	if(!this->default_category)
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "No default terminal affectation defined, "
		    "some terminals may not be able to log\n");
	}

	this->is_parent_init = true;

	if (!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "the output probes and stats initialization have "
		    "failed\n");
		return false;
	}

	return true;
error:
	return false;
}

bool DamaCtrl::initOutput()
{
	// RBDC request number
	this->probe_gw_rbdc_req_num = Output::registerProbe<int>(
		"", true, SAMPLE_LAST, "Spot_%d.NCC.RBDC.RBDC request number", this->spot_id);
	this->gw_rbdc_req_num = 0;

	// RBDC requested capacity
	this->probe_gw_rbdc_req_size = Output::registerProbe<int>(
		"Kbits/s", true, SAMPLE_LAST, "Spot_%d.NCC.RBDC.RBDC requested capacity", this->spot_id);
	this->gw_rbdc_req_size_pktpf = 0;

	// VBDC request number
	this->probe_gw_vbdc_req_num = Output::registerProbe<int>(
		"", true, SAMPLE_LAST, "Spot_%d.NCC.VBDC.VBDC request number", this->spot_id);
	this->gw_vbdc_req_num = 0;

	// VBDC Requested capacity
	this->probe_gw_vbdc_req_size = Output::registerProbe<int>(
		"Kbits", true, SAMPLE_LAST, "Spot_%d.NCC.VBDC.VBDC requested capacity", this->spot_id);
	this->gw_vbdc_req_size_pkt = 0;

	// Allocated ressources
		// CRA allocation
	this->probe_gw_cra_alloc = Output::registerProbe<int>(
		"Kbits/s", true, SAMPLE_LAST, "Spot_%d.NCC.CRA allocated", this->spot_id);
	this->gw_cra_alloc_kbps = 0;

		// RBDC max
	this->probe_gw_rbdc_max = Output::registerProbe<int>(
		"Kbits/s", true, SAMPLE_LAST, "Spot_%d.NCC.RBDC.RBDC max", this->spot_id);
	this->gw_rbdc_max_kbps = 0;

		// RBDC allocation
	this->probe_gw_rbdc_alloc = Output::registerProbe<int>(
		"Kbits/s", true, SAMPLE_LAST, "Spot_%d.NCC.RBDC.RBDC allocated", this->spot_id);
	this->gw_rbdc_alloc_pktpf = 0;

		// VBDC allocation
	this->probe_gw_vbdc_alloc = Output::registerProbe<int>(
		"Kbits", true, SAMPLE_LAST, "Spot_%d.NCC.VBDC.VBDC allocated", this->spot_id);
	this->gw_vbdc_alloc_pkt = 0;

		// FCA allocation
	if(this->fca_kbps != 0)
	{
		// only create FCA probe if it is enabled
		this->probe_gw_fca_alloc = Output::registerProbe<int>(
			"Kbits/s", true, SAMPLE_LAST, "Spot_%d.NCC.FCA allocated", this->spot_id);
		this->gw_fca_alloc_pktpf = 0;
	}

		// Total and remaining capacity
	this->probe_gw_return_total_capacity = Output::registerProbe<int>(
		"Kbits/s", true, SAMPLE_LAST, "Spot_%d.Up/Return capacity.Total.Available", this->spot_id);
	this->gw_return_total_capacity_pktpf = 0;
	this->probe_gw_return_remaining_capacity = Output::registerProbe<int>(
		"Kbits/s", true, SAMPLE_LAST, "Spot_%d.Up/Return capacity.Total.Remaining", this->spot_id);
	this->gw_remaining_capacity_pktpf = 0;

		// Logged ST number
	this->probe_gw_st_num = Output::registerProbe<int>(
		"", true, SAMPLE_LAST, "Spot_%d.NCC.ST number",this->spot_id );
	this->gw_st_num = 0;

	// Register output probes for simulated STs
	if(this->simulated)
	{
		Probe<int> *probe_cra;
		Probe<int> *probe_rbdc_max;
		Probe<int> *probe_rbdc;
		Probe<int> *probe_vbdc;
		Probe<int> *probe_fca;

		// tal_id 0 is for GW so it is unused
		tal_id_t tal_id = 0;
		probe_cra = Output::registerProbe<int>("Kbits/s", true, SAMPLE_MAX,
		                                       "Spot_%d.Simulated_ST.CRA allocation");
		this->probes_st_cra_alloc.insert(
			pair<tal_id_t, Probe<int> *>(tal_id, probe_cra));
		probe_rbdc_max = Output::registerProbe<int>("Kbits/s", true, SAMPLE_MAX,
		                                            "Spot_%d.Simulated_ST.RBDC max");
		this->probes_st_rbdc_max.insert(
			pair<tal_id_t, Probe<int> *>(tal_id, probe_rbdc_max));
		probe_rbdc = Output::registerProbe<int>("Kbits/s", true, SAMPLE_MAX,
		                                        "Spot_%d.Simulated_ST.RBDC allocation");
		this->probes_st_rbdc_alloc.insert(
			pair<tal_id_t, Probe<int> *>(tal_id, probe_rbdc));
		probe_vbdc = Output::registerProbe<int>("Kbits", true, SAMPLE_SUM,
		                                        "Spot_%d.Simulated_ST.VBDC allocation");
		this->probes_st_vbdc_alloc.insert(
			pair<tal_id_t, Probe<int> *>(tal_id, probe_vbdc));

		// only create FCA probe if it is enabled
		if(this->fca_kbps != 0)
		{
			probe_fca = Output::registerProbe<int>("Kbits/s", true, SAMPLE_MAX,
			                                       "Spot_%d.Simulated_ST.FCA allocation");
			this->probes_st_fca_alloc.insert(
				pair<tal_id_t, Probe<int> *>(tal_id, probe_fca));
		}
	}

	return true;
}

bool DamaCtrl::hereIsLogon(const LogonRequest *logon)
{
	tal_id_t tal_id = logon->getMac();
	rate_kbps_t cra_kbps = logon->getRtBandwidth();
	rate_kbps_t max_rbdc_kbps = logon->getMaxRbdc();
	vol_kb_t max_vbdc_kb = logon->getMaxVbdc();
	LOG(this->log_logon, LEVEL_INFO,
	    "New ST: #%u, with CRA: %u bits/sec\n", tal_id, cra_kbps);

	DamaTerminalList::iterator it;
	it = this->terminals.find(tal_id);
	if(it == this->terminals.end())
	{
		TerminalContextDama *terminal;
		TerminalMapping<TerminalCategoryDama>::const_iterator it;
		TerminalCategories<TerminalCategoryDama>::const_iterator category_it;
		TerminalCategoryDama *category;
		vector<CarriersGroupDama *> carriers;
		vector<CarriersGroupDama *>::const_iterator carrier_it;
		uint32_t max_capa_kbps = 0;

		// Find the associated category
		it = this->terminal_affectation.find(tal_id);
		if(it == this->terminal_affectation.end())
		{
			if(!this->default_category)
			{
				LOG(this->log_logon, LEVEL_WARNING,
				    "ST #%u cannot be logged, there is no default category\n",
				    tal_id);
				return false;
			}
			LOG(this->log_logon, LEVEL_INFO,
			    "ST #%d is not affected to a category, using "
			    "default: %s\n", tal_id,
			    this->default_category->getLabel().c_str());
			category = this->default_category;
		}
		else
		{
			category = (*it).second;
		}
		if(!category)
		{
			LOG(this->log_logon, LEVEL_WARNING,
			    "ST #%u cannot be logged, there is no DAMA "
			    "category associated to its mapping\n",
			    tal_id);
			return false;
		}

		// check if the category is concerned by DAMA
		if(this->categories.find(category->getLabel()) == this->categories.end())
		{
			LOG(this->log_logon, LEVEL_INFO,
			    "Terminal %u is affected to non DAMA category\n", tal_id);
			return true;
		}

		// create the terminal
		if(!this->createTerminal(&terminal,
		                         tal_id,
		                         cra_kbps,
		                         max_rbdc_kbps,
		                         this->rbdc_timeout_sf,
		                         max_vbdc_kb))
		{
			LOG(this->log_logon, LEVEL_ERROR,
			    "Cannot create terminal context for ST #%d\n",
			    tal_id);
			return false;
		}

		if(tal_id < BROADCAST_TAL_ID)
		{
			Probe<int> *probe_cra;
			Probe<int> *probe_rbdc_max;
			Probe<int> *probe_rbdc;
			Probe<int> *probe_vbdc;
			Probe<int> *probe_fca;

			// Output probes and stats
			probe_cra = Output::registerProbe<int>("Kbits/s", true, SAMPLE_MAX,
			                                       "ST%u_allocation.CRA allocation",
			                                       tal_id);
			this->probes_st_cra_alloc.insert(
				pair<tal_id_t, Probe<int> *>(tal_id, probe_cra));
			probe_rbdc_max = Output::registerProbe<int>("Kbits/s", true, SAMPLE_MAX,
			                                            "ST%u_allocation.RBDC max",
			                                            tal_id);
			this->probes_st_rbdc_max.insert(
				pair<tal_id_t, Probe<int> *>(tal_id, probe_rbdc_max));
			probe_rbdc = Output::registerProbe<int>("Kbits/s", true, SAMPLE_MAX,
			                                        "ST%u_allocation.RBDC allocation",
			                                        tal_id);
			this->probes_st_rbdc_alloc.insert(
				pair<tal_id_t, Probe<int> *>(tal_id, probe_rbdc));
			probe_vbdc = Output::registerProbe<int>("Kbits", true, SAMPLE_SUM,
			                                        "ST%u_allocation.VBDC allocation",
			                                        tal_id);
			this->probes_st_vbdc_alloc.insert(
				pair<tal_id_t, Probe<int> *>(tal_id, probe_vbdc));
			// only create FCA probe if it is enabled
			if(this->fca_kbps != 0)
			{
				probe_fca = Output::registerProbe<int>("Kbits/s", true, SAMPLE_MAX,
				                                       "ST%u_allocation.FCA allocation",
				                                       tal_id);
				this->probes_st_fca_alloc.insert(
					pair<tal_id_t, Probe<int> *>(tal_id, probe_fca));
			}
		}

		// Add the new terminal to the list
		this->terminals.insert(
			pair<unsigned int, TerminalContextDama *>(tal_id, terminal));

		// add terminal in category and inform terminal of its category
		category->addTerminal(terminal);
		terminal->setCurrentCategory(category->getLabel());
		LOG(this->log_logon, LEVEL_NOTICE,
		    "Add terminal %u in category %s\n",
		    tal_id, category->getLabel().c_str());
		if(tal_id > BROADCAST_TAL_ID)
		{
			DC_RECORD_EVENT("LOGON st%d rt=%u rbdc=%u vbdc=%u", logon->getMac(),
			                logon->getRtBandwidth(), max_rbdc_kbps, max_vbdc_kb);
		}

		// Output probes and stats
		this->gw_st_num += 1;
		this->gw_cra_alloc_kbps += cra_kbps;
		this->probe_gw_cra_alloc->put(gw_cra_alloc_kbps);
		this->gw_rbdc_max_kbps += max_rbdc_kbps;
		this->probe_gw_rbdc_max->put(gw_rbdc_max_kbps);

		// check that CRA is not too high, else print a warning !
		carriers = category->getCarriersGroups();
		for(carrier_it = carriers.begin();
		    carrier_it != carriers.end();
		    ++carrier_it)
		{
			// we can use the same function that convert sym to kbits
			// for conversion from sym/s to kbits/s
			max_capa_kbps +=
					// the last FMT ID in getFmtIds() is the one
					// which will give us the higher rate
					this->input_modcod_def->symToKbits((*carrier_it)->getFmtIds().back(),
					                       (*carrier_it)->getSymbolRate() *
					                       (*carrier_it)->getCarriersNumber());

		}

		if(cra_kbps > max_capa_kbps)
		{
			LOG(this->log_logon, LEVEL_WARNING,
			    "The CRA value for ST%u is too high compared to "
			    "the maximum carrier capacity (%u > %u)\n",
			    tal_id, cra_kbps, max_capa_kbps);
		}

	}
	else
	{
		LOG(this->log_logon, LEVEL_NOTICE,
		    "Duplicate logon received for ST #%u\n", tal_id);
	}


	return true;
}

bool DamaCtrl::hereIsLogoff(const Logoff *logoff)
{
	DamaTerminalList::iterator it;
	TerminalContextDama *terminal;
	TerminalCategories<TerminalCategoryDama>::const_iterator category_it;
	TerminalCategoryDama *category;
	tal_id_t tal_id = logoff->getMac();

	it = this->terminals.find(tal_id);
	if(it == this->terminals.end())
	{
		LOG(this->log_logon, LEVEL_INFO,
		    "No ST found for id %u\n", tal_id);
		return false;
	}

	terminal = (*it).second;

	// Output probes and stats
	this->gw_st_num -= 1;
	this->gw_cra_alloc_kbps -= terminal->getCra();
	this->probe_gw_cra_alloc->put(this->gw_cra_alloc_kbps);
	this->gw_rbdc_max_kbps -= terminal->getMaxRbdc();
	this->probe_gw_rbdc_max->put(this->gw_rbdc_max_kbps);

	// remove terminal from the list
	this->terminals.erase(terminal->getTerminalId());

	// remove terminal from the terminal category
	category_it = this->categories.find(terminal->getCurrentCategory());
	if(category_it != this->categories.end())
	{
		category = (*category_it).second;
		if(!category->removeTerminal(terminal))
		{
			return false;
		}
	}

	if(tal_id > BROADCAST_TAL_ID)
	{
		DC_RECORD_EVENT("LOGOFF st%d", tal_id);
	}

	return true;
}

bool DamaCtrl::runOnSuperFrameChange(time_sf_t superframe_number_sf)
{
	DamaTerminalList::iterator it;

	this->current_superframe_sf = superframe_number_sf;

	for(it = this->terminals.begin(); it != this->terminals.end(); it++)
	{
		TerminalContextDama *terminal = it->second;
		// reset/update terminal allocations/requests
		terminal->onStartOfFrame();
	}

	if(!this->runDama())
	{
		LOG(this->log_super_frame_tick, LEVEL_ERROR,
		    "Error during DAMA computation.\n");
		return false;
	}

	return 0;
}


bool DamaCtrl::runDama()
{
	DamaTerminalList::iterator tal_it;

	// reset the DAMA settings
	if(!this->resetDama())
	{
		LOG(this->log_run_dama, LEVEL_ERROR,
		    "SF#%u: Cannot reset DAMA\n",
		    this->current_superframe_sf);
		return false;
	}

	if(this->enable_rbdc && !this->runDamaRbdc())
	{
		LOG(this->log_run_dama, LEVEL_ERROR,
		    "SF#%u: Error while computing RBDC allocation\n",
		    this->current_superframe_sf);
		return false;
	}
	else
	{
		// Output stats and probes
		for(tal_it = this->terminals.begin(); tal_it != this->terminals.end(); ++tal_it)
		{
			tal_id_t tal_id = tal_it->first;
			if(tal_id < BROADCAST_TAL_ID)
			{
				this->probes_st_rbdc_alloc[tal_id]->put(0);
			}
		}
		if(this->simulated)
		{
			this->probes_st_rbdc_alloc[0]->put(0);
		}
		this->probe_gw_rbdc_req_num->put(0);
		this->probe_gw_rbdc_req_size->put(0);
		this->probe_gw_rbdc_alloc->put(0);
	}
	if(this->enable_vbdc && !this->runDamaVbdc())
	{
		LOG(this->log_run_dama, LEVEL_ERROR,
		    "SF#%u: Error while computing RBDC allocation\n",
		    this->current_superframe_sf);
		return false;
	}
	else
	{
		// Output stats and probes
		for(tal_it = this->terminals.begin(); tal_it != this->terminals.end(); ++tal_it)
		{
			tal_id_t tal_id = tal_it->first;
			if(tal_id < BROADCAST_TAL_ID)
			{
				this->probes_st_vbdc_alloc[tal_id]->put(0);
			}
		}
		if(this->simulated)
		{
			this->probes_st_vbdc_alloc[0]->put(0);
		}


		this->probe_gw_vbdc_req_num->put(0);
		this->probe_gw_vbdc_req_size->put(0);
		this->probe_gw_vbdc_alloc->put(0);
	}
	if(!this->runDamaFca())
	{
		LOG(this->log_run_dama, LEVEL_ERROR,
		    "SF#%u: Error while computing RBDC allocation\n",
		    this->current_superframe_sf);
		return false;
	}
	return true;
}

void DamaCtrl::setRecordFile(FILE *event_stream)
{
	this->event_file = event_stream;
	DC_RECORD_EVENT("%s", "# --------------------------------------\n");
}


// TODO disable timers on probes if output is disabled
// and event to reactivate them ?!
void DamaCtrl::updateStatistics(time_ms_t UNUSED(period_ms))
{
	int simu_cra = 0;
	int simu_rbdc = 0;
	TerminalCategories<TerminalCategoryDama>::iterator cat_it;
	// Update probes and stats
	this->probe_gw_st_num->put(this->gw_st_num);
	this->probe_gw_cra_alloc->put(this->gw_cra_alloc_kbps);
	this->probe_gw_rbdc_max->put(this->gw_rbdc_max_kbps);
	for(DamaTerminalList::iterator it = this->terminals.begin();
	    it != this->terminals.end(); ++it)
	{
		tal_id_t tal_id = it->first;
		TerminalContextDama *terminal = it->second;
		if(tal_id > BROADCAST_TAL_ID)
		{
			simu_cra += terminal->getCra();
			simu_rbdc += terminal->getMaxRbdc();
		}
		else
		{
			this->probes_st_cra_alloc[tal_id]->put(
				terminal->getCra());
			this->probes_st_rbdc_max[tal_id]->put(
				terminal->getMaxRbdc());
		}
	}
	if(this->simulated)
	{
		this->probes_st_cra_alloc[0]->put(simu_cra);
		this->probes_st_rbdc_max[0]->put(simu_rbdc);
	}
	this->probe_gw_return_remaining_capacity->put(
		this->converter->pktpfToKbps(this->gw_remaining_capacity_pktpf));
	for(cat_it = this->categories.begin();
	    cat_it != categories.end(); ++cat_it)
	{
		TerminalCategoryDama *category = (*cat_it).second;
		vector<CarriersGroupDama *> carriers;
		vector<CarriersGroupDama *>::const_iterator carrier_it;
		string label = category->getLabel();
		this->probes_category_return_remaining_capacity[label]->put(
			this->converter->pktpfToKbps(
				this->category_return_remaining_capacity_pktpf[label]));
		carriers = category->getCarriersGroups();
		for(carrier_it = carriers.begin();
			carrier_it != carriers.end(); ++carrier_it)
		{
			unsigned int carrier_id = (*carrier_it)->getCarriersId();
			this->probes_carrier_return_remaining_capacity[carrier_id]->put(
				this->converter->pktpfToKbps(
					this->carrier_return_remaining_capacity_pktpf[carrier_id]));
		}
	}
}
