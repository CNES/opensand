/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file BlockDvbNcc.cpp
 * @brief This bloc implements a DVB-S/RCS stack for a Ncc.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#include "BlockDvbNcc.h"

#include "DamaCtrlRcsLegacy.h"

#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "DvbScpcStd.h"
#include "Sof.h"
#include "ForwardSchedulingS2.h"
#include "UplinkSchedulingRcs.h"

#include <opensand_output/Output.h>

#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/times.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ios>

/*
 * REMINDER:
 *  // in transparent mode
 *        - downward => forward link
 *        - upward => return link
 *  // in regenerative mode
 *        - downward => uplink
 *        - upward => downlink
 */

/*****************************************************************************/
/*                                Block                                      */
/*****************************************************************************/


BlockDvbNcc::BlockDvbNcc(const string &name):
	BlockDvb(name)
{
}

BlockDvbNcc::~BlockDvbNcc()
{
}

bool BlockDvbNcc::onInit(void)
{
	return true;
}


bool BlockDvbNcc::onDownwardEvent(const RtEvent *const event)
{
	return ((Downward *)this->downward)->onEvent(event);
}


bool BlockDvbNcc::onUpwardEvent(const RtEvent *const event)
{
	return ((Upward *)this->upward)->onEvent(event);
}

/*****************************************************************************/
/*                              Downward                                     */
/*****************************************************************************/


// TODO lot of duplicated code for fifos between ST and GW

BlockDvbNcc::Downward::Downward(Block *const bl):
	DvbDownward(bl),
	NccPepInterface(),
	dama_ctrl(NULL),
	scheduling(NULL),
	fwd_timer(-1),
	fwd_frame_counter(0),
	ctrl_carrier_id(),
	sof_carrier_id(),
	data_carrier_id(),
	dvb_fifos(),
	default_fifo_id(0),
	complete_dvb_frames(),
	categories(),
	terminal_affectation(),
	default_category(NULL),
	up_return_pkt_hdl(NULL),
	fwd_fmt_groups(),
	ret_fmt_groups(),
	up_ret_fmt_simu(),
	down_fwd_fmt_simu(),
	scenario_timer(-1),
	cni(100),
	column_list(),
	pep_cmd_apply_timer(-1),
	pep_alloc_delay(-1),
	event_file(NULL),
	simu_file(NULL),
	simulate(none_simu),
	simu_st(-1),
	simu_rt(-1),
	simu_max_rbdc(-1),
	simu_max_vbdc(-1),
	simu_cr(-1),
	simu_interval(-1),
	simu_eof(false),
	simu_timer(-1),
	probe_gw_queue_size(),
	probe_gw_queue_size_kb(),
	probe_gw_queue_loss(),
	probe_gw_queue_loss_kb(),
	probe_gw_l2_to_sat_before_sched(),
	probe_gw_l2_to_sat_after_sched(),
	probe_gw_l2_to_sat_total(NULL),
	l2_to_sat_total_bytes(0),
	probe_frame_interval(NULL),
	probe_used_modcod(NULL),
	log_request_simulation(NULL),
	event_logon_resp(NULL)
{
}

BlockDvbNcc::Downward::~Downward()
{
	if(this->dama_ctrl)
		delete this->dama_ctrl;
	if(this->scheduling)
		delete this->scheduling;

	this->complete_dvb_frames.clear();

	if(this->event_file)
	{
		fflush(this->event_file);
		fclose(this->event_file);
	}
	if(this->simu_file)
	{
		fclose(this->simu_file);
	}
	// delete FMT groups here because they may be present in many carriers
	// TODO do something to avoid groups here
	for(fmt_groups_t::iterator it = this->fwd_fmt_groups.begin();
	    it != this->fwd_fmt_groups.end(); ++it)
	{
		delete (*it).second;
	}
	// delete FMT groups here because they may be present in many carriers
	// TODO do something to avoid groups here
	for(fmt_groups_t::iterator it = this->ret_fmt_groups.begin();
	    it != this->ret_fmt_groups.end(); ++it)
	{
		delete (*it).second;
	}

	// delete fifos
	for(fifos_t::iterator it = this->dvb_fifos.begin();
		it != this->dvb_fifos.end(); ++it)
	{
		delete (*it).second;
	}
	this->dvb_fifos.clear();
		
	if(this->satellite_type == TRANSPARENT)
	{
		TerminalCategories<TerminalCategoryDama>::iterator cat_it;
		for(cat_it = this->categories.begin();
		    cat_it != this->categories.end(); ++cat_it)
		{
			delete (*cat_it).second;
		}
		this->categories.clear();
	}

	this->terminal_affectation.clear();
}


bool BlockDvbNcc::Downward::onInit(void)
{
	const char *scheme;
	
	if(!this->initSatType())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed get satellite type\n");
		goto error;
	}

	// get the common parameters
	if(this->satellite_type == TRANSPARENT)
	{
		scheme = FORWARD_DOWN_ENCAP_SCHEME_LIST;
	}
	else
	{
		scheme = RETURN_UP_ENCAP_SCHEME_LIST;
	}

	if(!this->initCommon(scheme))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		goto error;
	}
	if(!this->initDown())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the downward common "
		    "initialisation\n");
		goto error;
	}
	
	if(this->satellite_type == REGENERATIVE)
	{
		this->up_return_pkt_hdl = this->pkt_hdl;
	}
	else
	{
		if(!this->initPktHdl(RETURN_UP_ENCAP_SCHEME_LIST,
		                     &this->up_return_pkt_hdl, false))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed get packet handler\n");
			goto error;
		}

	}

	if(!this->initRequestSimulation())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the request simulation part of "
		    "the initialisation\n");
		goto error;
	}

	// Get the carrier Ids
	if(!this->initCarrierIds())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the carrier IDs part of the "
		    "initialisation\n");
		goto error;
	}

	if(!this->initFifo())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the FIFO part of the "
		    "initialisation\n");
		goto release_dama;
	}

	// Get and open the files
	if(!this->initModcodSimu())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the files part of the "
		    "initialisation\n");
		goto error;
	}

	if(!this->initMode())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the mode part of the "
		    "initialisation\n");
		goto error;
	}

	// get and launch the dama algorithm
	if(!this->initDama())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the DAMA part of the "
		    "initialisation\n");
		goto error;
	}

	this->initStatsTimer(this->fwd_down_frame_duration_ms);

	if(!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialization of "
		    "statistics\n");
		goto release_dama;
	}

	// initialize the timers
	if(!this->initTimers())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the timers part of the "
		    "initialisation\n");
		goto release_dama;
	}

	// initialize the column ID for FMT simulation
	if(!this->initColumns())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the columns ID for FMT "
		    "simulation\n");
		goto release_dama;
	}

	// listen for connections from external PEP components
	if(!this->listenForPepConnections())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to listen for PEP connections\n");
		goto release_dama;
	}
	this->addNetSocketEvent("pep_listen", this->getPepListenSocket(), 200);

	// everything went fine
	return true;

release_dama:
	delete this->dama_ctrl;
error:
	return false;
}

bool BlockDvbNcc::Downward::initRequestSimulation(void)
{
	string str_config;

	memset(this->simu_buffer, '\0', SIMU_BUFF_LEN);
	// Get and open the event file
	if(!Conf::getValue(DVB_NCC_SECTION, DVB_EVENT_FILE, str_config))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot load parameter %s from section %s\n",
		    DVB_EVENT_FILE, DVB_NCC_SECTION);
		goto error;
	}
	if(str_config != "none" && this->with_phy_layer)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot use simulated request with physical layer "
		    "because we need to add cni parameters in SAC "
		    "(TBD!)\n");
		goto error;
	}

	if(str_config ==  "stdout")
	{
		this->event_file = stdout;
	}
	else if(str_config == "stderr")
	{
		this->event_file = stderr;
	}
	else if(str_config != "none")
	{
		this->event_file = fopen(str_config.c_str(), "a");
		if(this->event_file == NULL)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "%s\n", strerror(errno));
		}
	}
	if(this->event_file == NULL && str_config != "none")
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "no record file will be used for event\n");
	}
	else if(this->event_file != NULL)
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "events recorded in %s.\n", str_config.c_str());
	}

	// Get and set simulation parameter
	//
	this->simulate = none_simu;
	if(!Conf::getValue(DVB_NCC_SECTION, DVB_SIMU_MODE, str_config))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot load parameter %s from section %s\n",
		    DVB_SIMU_MODE, DVB_NCC_SECTION);
		goto error;
	}

	// TODO for stdin use FileEvent for simu_timer ?
	if(str_config == "file")
	{
		if(!Conf::getValue(DVB_NCC_SECTION, DVB_SIMU_FILE, str_config))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot load parameter %s from section %s\n",
			    DVB_SIMU_FILE, DVB_NCC_SECTION);
			goto error;
		}
		if(str_config == "stdin")
		{
			this->simu_file = stdin;
		}
		else
		{
			this->simu_file = fopen(str_config.c_str(), "r");
		}
		if(this->simu_file == NULL && str_config != "none")
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "%s\n", strerror(errno));
			LOG(this->log_init, LEVEL_ERROR,
			    "no simulation file will be used.\n");
		}
		else
		{
			LOG(this->log_init, LEVEL_NOTICE,
			    "events simulated from %s.\n",
			    str_config.c_str());
			this->simulate = file_simu;
			this->simu_timer = this->addTimerEvent("simu_file",
			                                       this->ret_up_frame_duration_ms);
		}
	}
	else if(str_config == "random")
	{
		int val;

		if(!Conf::getValue(DVB_NCC_SECTION, DVB_SIMU_RANDOM, str_config))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot load parameter %s from section %s\n",
			    DVB_SIMU_RANDOM, DVB_NCC_SECTION);
            goto error;
		}
		val = sscanf(str_config.c_str(), "%ld:%ld:%ld:%ld:%ld:%ld",
		             &this->simu_st, &this->simu_rt, &this->simu_max_rbdc,
		             &this->simu_max_vbdc, &this->simu_cr, &this->simu_interval);
		if(val < 4)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot load parameter %s from section %s\n",
			    DVB_SIMU_RANDOM, DVB_NCC_SECTION);
			goto error;
		}
		else
		{
			LOG(this->log_init, LEVEL_NOTICE,
			    "random events simulated for %ld terminals with "
			    "%ld kb/s bandwidth, %ld kb/s max RBDC, "
			    "%ld kb max VBDC, a mean request of %ld kb/s "
			    "and a request amplitude of %ld kb/s)",
			    this->simu_st, this->simu_rt, this->simu_max_rbdc,
			    this->simu_max_vbdc, this->simu_cr,
			    this->simu_interval);
		}
		this->simulate = random_simu;
		this->simu_timer = this->addTimerEvent("simu_random",
		                                       this->ret_up_frame_duration_ms);
		srandom(times(NULL));
	}
	else
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "no event simulation\n");
	}

    return true;

error:
    return false;
}


bool BlockDvbNcc::Downward::initTimers(void)
{
	// Set #sf and launch frame timer
	this->super_frame_counter = 0;
	this->frame_timer = this->addTimerEvent("frame",
	                                        this->ret_up_frame_duration_ms);
	this->fwd_timer = this->addTimerEvent("fwd_timer",
	                                      this->fwd_down_frame_duration_ms);

	// Launch the timer in order to retrieve the modcods if there is no physical layer
	// or to send SAC with ACM parameters in regenerative mode
	if(!this->with_phy_layer || this->satellite_type == REGENERATIVE)
	{
		this->scenario_timer = this->addTimerEvent("scenario",
		                                           this->dvb_scenario_refresh);
	}

	// read the pep allocation delay
	if(!Conf::getValue(NCC_SECTION_PEP, DVB_NCC_ALLOC_DELAY,
	                   this->pep_alloc_delay))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    NCC_SECTION_PEP, DVB_NCC_ALLOC_DELAY);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "pep_alloc_delay set to %d ms\n", this->pep_alloc_delay);
	// create timer
	this->pep_cmd_apply_timer = this->addTimerEvent("pep_request",
	                                                pep_alloc_delay,
	                                                false, // no rearm
	                                                false // do not start
	                                                );

	return true;

error:
	return false;
}

bool BlockDvbNcc::Downward::initColumns(void)
{
	int i = 0;
	ConfigurationList columns;
	ConfigurationList::iterator iter;

	// Get the list of STs
	if(!Conf::getListItems(SAT_SIMU_COL_SECTION, COLUMN_LIST,
	                       columns))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s, %s': problem retrieving simulation "
		    "column list\n", SAT_SIMU_COL_SECTION, COLUMN_LIST);
		goto error;
	}

	for(iter = columns.begin(); iter != columns.end(); iter++)
	{
		i++;
		uint16_t tal_id;
		uint16_t column_nbr;

		// Get the Tal ID
		if(!Conf::getAttributeValue(iter, TAL_ID, tal_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "problem retrieving %s in simulation column "
			    "entry %d\n", TAL_ID, i);
			goto error;
		}
		// Get the column nbr
		if(!Conf::getAttributeValue(iter, COLUMN_NBR, column_nbr))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "problem retrieving %s in simulation column "
			    "entry %d\n", COLUMN_NBR, i);
			goto error;
		}

		this->column_list[tal_id] = column_nbr;
	}

	if(this->column_list.find(GW_TAL_ID) == this->column_list.end())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "GW is not declared in column IDs\n");
		goto error;
	}

	// declare the GW as one ST for the MODCOD scenarios
	if(!this->up_ret_fmt_simu.addTerminal(GW_TAL_ID,
	                                      this->column_list[GW_TAL_ID]) ||
	   !this->down_fwd_fmt_simu.addTerminal(GW_TAL_ID,
	                                        this->column_list[GW_TAL_ID]))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to define the GW as ST with ID %d\n",
		    GW_TAL_ID);
		goto error;
	}

	return true;

error:
	return false;
}


bool BlockDvbNcc::Downward::initMode(void)
{
	TerminalCategoryDama *cat;

	// initialize scheduling
	// depending on the satellite type
	if(this->satellite_type == TRANSPARENT)
	{
		if(!this->initBand<TerminalCategoryDama>(FORWARD_DOWN_BAND,
		                                         TDM,
		                                         this->fwd_down_frame_duration_ms,
		                                         this->satellite_type,
		                                         this->down_fwd_fmt_simu.getModcodDefinitions(),
		                                         this->categories,
		                                         this->terminal_affectation,
		                                         &this->default_category,
		                                         this->fwd_fmt_groups))
		{
			return false;
		}

		if(this->categories.size() != 1)
		{
			// TODO at the moment we use only one category
			// To implement more than one category we will need to create one (a group of)
			// fifo(s) per category and schedule per (group of) fifo(s).
			// The packets would then be pushed in the correct (group of) fifo(s) according to
			// the category the destination terminal ID belongs
			// this is why we have categories, terminal_affectation and default_category
			// as attributes
			// map<cat label, sched> and fifos in scheduler ?
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot support more than one category for "
			    "down/forward band\n");
			return false;
		}

		cat = this->categories.begin()->second;
		this->scheduling = new ForwardSchedulingS2(this->fwd_down_frame_duration_ms,
		                                           this->pkt_hdl,
		                                           this->dvb_fifos,
		                                           &this->down_fwd_fmt_simu,
		                                           cat);
	}
	else if(this->satellite_type == REGENERATIVE)
	{
		if(!this->initBand<TerminalCategoryDama>(RETURN_UP_BAND,
		                                         DAMA,
		                                         this->ret_up_frame_duration_ms, 
		                                         this->satellite_type,
		                                         this->up_ret_fmt_simu.getModcodDefinitions(),
		                                         this->categories,
		                                         this->terminal_affectation,
		                                         &this->default_category,
		                                         this->ret_fmt_groups))
		{
			return false;
		}

		// here we need the category to which the GW belongs
		if(this->terminal_affectation.find(GW_TAL_ID) != this->terminal_affectation.end())
		{
			cat = this->terminal_affectation[GW_TAL_ID];
		}
		else
		{
			if(!this->default_category)
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "No default category and GW has no affectation\n");
				return false;
			}
			cat = this->default_category;
		}
		this->scheduling = new UplinkSchedulingRcs(this->pkt_hdl,
		                                           this->dvb_fifos,
		                                           &this->up_ret_fmt_simu,
		                                           cat);
	}
	else
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "unknown value '%u' for satellite type ",
		    this->satellite_type);
		goto error;

	}
	if(!this->scheduling)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to create the scheduling\n");
		goto error;
	}

	return true;

error:
	return false;
}

bool BlockDvbNcc::Downward::initCarrierIds(void)
{
	// Get the ID for DVB control carrier
	if(!Conf::getValue(SATCAR_SECTION,
	                   DVB_CTRL_CAR,
	                   this->ctrl_carrier_id))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SATCAR_SECTION, DVB_CTRL_CAR);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "DVB control carrier ID set to %u\n",
	    this->ctrl_carrier_id);

	// Get the ID for SOF carrier
	if(!Conf::getValue(SATCAR_SECTION,
	                   DVB_SOF_CAR,
	                   this->sof_carrier_id))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SATCAR_SECTION, DVB_SOF_CAR);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "SoF carrier ID set to %u\n", this->sof_carrier_id);

	// Get the ID for data carrier
	if(!Conf::getValue(SATCAR_SECTION,
	                   DVB_DATA_CAR,
	                   this->data_carrier_id))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SATCAR_SECTION, DVB_DATA_CAR);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "Data carrier ID set to %u\n", this->data_carrier_id);

	return true;

error:
	return false;
}


bool BlockDvbNcc::Downward::initModcodSimu(void)
{
	if(!this->initModcodFiles(RETURN_UP_MODCOD_DEF_RCS,
	                          RETURN_UP_MODCOD_TIME_SERIES,
	                          this->up_ret_fmt_simu))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the up/return MODCOD files\n");
		goto error;
	}
	if(!this->initModcodFiles(FORWARD_DOWN_MODCOD_DEF_S2,
	                          FORWARD_DOWN_MODCOD_TIME_SERIES,
	                          this->down_fwd_fmt_simu))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the forward MODCOD files\n");
		goto error;
	}

	// initialize the MODCOD IDs
	if(!this->down_fwd_fmt_simu.goNextScenarioStep(true) ||
	   !this->up_ret_fmt_simu.goNextScenarioStep(false))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize MODCOD scheme IDs\n");
		goto error;
	}

	return true;

error:
	return false;
}


// TODO this function is NCC part but other functions are related to GW,
//      we could maybe create two classes inside the block to keep them separated
bool BlockDvbNcc::Downward::initDama(void)
{
	bool cra_decrease;
	time_ms_t sync_period_ms;
	time_frame_t sync_period_frame;
	time_sf_t rbdc_timeout_sf;
	rate_kbps_t fca_kbps;
	string dama_algo;

	TerminalCategories<TerminalCategoryDama> dc_categories;
	TerminalMapping<TerminalCategoryDama> dc_terminal_affectation;
	TerminalCategoryDama *dc_default_category;

	// Retrieving the cra decrease parameter
	if(!Conf::getValue(DC_SECTION_NCC, DC_CRA_DECREASE, cra_decrease))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "missing %s parameter", DC_CRA_DECREASE);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE, "cra_decrease = %s\n",
	    cra_decrease == true ? "true" : "false");

	// Retrieving the free capacity assignement parameter
	if(!Conf::getValue(DC_SECTION_NCC, DC_FREE_CAP, fca_kbps))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "missing %s parameter", DC_FREE_CAP);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "fca = %d kb/s\n", fca_kbps);

	if(!Conf::getValue(GLOBAL_SECTION, SYNC_PERIOD,
	                   sync_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s", SYNC_PERIOD);
		goto error;
	}
	sync_period_frame = (time_frame_t)round((double)sync_period_ms /
	                                        (double)this->ret_up_frame_duration_ms);
	rbdc_timeout_sf = sync_period_frame + 1;

	LOG(this->log_init, LEVEL_NOTICE,
	    "rbdc_timeout = %d superframes computed from sync period %d superframes\n",
	    rbdc_timeout_sf, sync_period_frame);

	if(this->satellite_type == TRANSPARENT)
	{
		if(!this->initBand<TerminalCategoryDama>(RETURN_UP_BAND,
		                                         DAMA,
		                                         this->ret_up_frame_duration_ms,
		                                         this->satellite_type,
		                                         this->up_ret_fmt_simu.getModcodDefinitions(),
		                                         dc_categories,
		                                         dc_terminal_affectation,
		                                         &dc_default_category,
		                                         this->ret_fmt_groups))
		{
			return false;
		}
	}
	else
	{
		// band already initialized in initMode
		dc_categories = this->categories;
		dc_terminal_affectation = this->terminal_affectation;
		dc_default_category = this->default_category;
	}

	// check if there is DAMA carriers
	if(dc_categories.size() == 0)
	{
		if(this->satellite_type == REGENERATIVE)
		{
			// No Slotted Aloha with regenerative satellite,
			// so we need a DAMA
			LOG(this->log_init, LEVEL_ERROR,
			    "No DAMA and regenerative satellite\n");
			return false;
		}
		LOG(this->log_init, LEVEL_NOTICE,
		    "No TDM carrier, won't allocate DAMA\n");
		// Also disable request simulation
		this->simulate = none_simu;
		return true;
	}

	// dama algorithm
	if(!Conf::getValue(DVB_NCC_SECTION, DVB_NCC_DAMA_ALGO,
	                   dama_algo))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DVB_NCC_SECTION, DVB_NCC_DAMA_ALGO);
		goto error;
	}

	/* select the specified DAMA algorithm */
	// TODO create one DAMA per spot and add spot_id as param ?
	if(dama_algo == "Legacy")
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "creating Legacy DAMA controller\n");
		this->dama_ctrl = new DamaCtrlRcsLegacy();
	}
	else
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': bad value for parameter '%s'\n",
		    DVB_NCC_SECTION, DVB_NCC_DAMA_ALGO);
		goto error;
	}

	if(!this->dama_ctrl)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to create the DAMA controller\n");
		goto error;
	}

	// Initialize the DamaCtrl parent class
	if(!this->dama_ctrl->initParent(this->ret_up_frame_duration_ms,
	                                this->with_phy_layer,
	                                this->up_return_pkt_hdl->getFixedLength(),
	                                cra_decrease,
	                                rbdc_timeout_sf,
	                                fca_kbps,
	                                dc_categories,
	                                dc_terminal_affectation,
	                                dc_default_category,
	                                &this->up_ret_fmt_simu,
	                                (this->simulate == none_simu) ?
	                                false : true))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Dama Controller Initialization failed.\n");
		goto release_dama;
	}

	if(!this->dama_ctrl->init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the DAMA controller\n");
		goto release_dama;
	}
	this->dama_ctrl->setRecordFile(this->event_file);

	return true;

release_dama:
	delete this->dama_ctrl;
error:
	return false;
}


bool BlockDvbNcc::Downward::initFifo(void)
{
	ConfigurationList fifo_list;
	ConfigurationList::iterator iter;

	/*
	* Read the MAC queues configuration in the configuration file.
	* Create and initialize MAC FIFOs
	*/
	if(!Conf::getListItems(DVB_NCC_SECTION, FIFO_LIST, fifo_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s, %s': missing fifo list", DVB_NCC_SECTION,
		    FIFO_LIST);
		goto err_fifo_release;
	}

	for(iter = fifo_list.begin(); iter != fifo_list.end(); iter++)
	{
		unsigned int fifo_priority;
		vol_pkt_t fifo_size = 0;
		string fifo_name;
		string fifo_access_type;
		DvbFifo *fifo;

		// get fifo_id --> fifo_priority
		if(!Conf::getAttributeValue(iter, FIFO_PRIO, fifo_priority))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_PRIO, DVB_NCC_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get fifo_name
		if(!Conf::getAttributeValue(iter, FIFO_NAME, fifo_name))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_NAME, DVB_NCC_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get fifo_size
		if(!Conf::getAttributeValue(iter, FIFO_SIZE, fifo_size))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_SIZE, DVB_NCC_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get the fifo CR type
		if(!Conf::getAttributeValue(iter, FIFO_ACCESS_TYPE, fifo_access_type))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_ACCESS_TYPE, DVB_NCC_SECTION,
			    FIFO_LIST);
			goto err_fifo_release;
		}

		fifo = new DvbFifo(fifo_priority, fifo_name,
		                   fifo_access_type, fifo_size);

		LOG(this->log_init, LEVEL_NOTICE,
		    "Fifo priority = %u, FIFO name %s, size %u, "
		    "access type %d\n",
		    fifo->getPriority(),
		    fifo->getName().c_str(),
		    fifo->getMaxSize(),
		    fifo->getAccessType());

		// the default FIFO is the last one = the one with the smallest priority
		// actually, the IP plugin should add packets in the default FIFO if
		// the DSCP field is not recognize, default_fifo_id should not be used
		// this is only used if traffic categories configuration and fifo configuration
		// are not coherent.
		this->default_fifo_id = std::max(this->default_fifo_id, fifo->getPriority());

		this->dvb_fifos.insert(pair<unsigned int, DvbFifo *>(fifo->getPriority(), fifo));
	} // end for(queues are now instanciated and initialized)

	//reset stat
	this->l2_to_sat_total_bytes = 0;

	return true;

err_fifo_release:
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		delete (*it).second;
	}
	this->dvb_fifos.clear();
	return false;

}

bool BlockDvbNcc::Downward::initOutput(void)
{
	// Events
	this->event_logon_resp = Output::registerEvent("DVB.logon_response");

	// Logs
	if(this->simulate != none_simu)
	{
		this->log_request_simulation = Output::registerLog(LEVEL_WARNING,
		                                                   "Dvb.RequestSimulation");
	}

	// Output probes and stats
	this->probe_frame_interval = Output::registerProbe<float>("Perf.Frames_interval",
	                                                          "ms", true,
	                                                          SAMPLE_LAST);
	for(fifos_t::iterator it = this->dvb_fifos.begin();
		it != this->dvb_fifos.end(); ++it)
	{
		const char *fifo_name = ((*it).second)->getName().data();
		unsigned int id = (*it).first;
			 
		this->probe_gw_queue_size[id] =
			Output::registerProbe<int>("Packets", true, SAMPLE_LAST,
		                               "Queue size.packets.%s", fifo_name);
		this->probe_gw_queue_size_kb[id] =
			Output::registerProbe<int>("kbits", true, SAMPLE_LAST,
		                               "Queue size.%s", fifo_name);
		this->probe_gw_l2_to_sat_before_sched[id] =
			Output::registerProbe<int>("Kbits/s", true, SAMPLE_AVG,
		                               "Throughputs.L2_to_SAT_before_sched.%s",
		                                fifo_name);
		this->probe_gw_l2_to_sat_after_sched[id] =
			Output::registerProbe<int>("Kbits/s", true, SAMPLE_AVG,
		                               "Throughputs.L2_to_SAT_after_sched.%s",
		                               fifo_name);
		this->probe_gw_queue_loss[id] =
			Output::registerProbe<int>("Packets", true, SAMPLE_SUM,
		                               "Queue loss.packets.%s",
		                               fifo_name);
		this->probe_gw_queue_loss_kb[id] =
			Output::registerProbe<int>("Kbits/s", true, SAMPLE_SUM,
		                               "Queue loss.%s",
		                               fifo_name);
	}
	this->probe_gw_l2_to_sat_total =
		Output::registerProbe<int>("Throughputs.L2_to_SAT_after_sched.total",
		                           "Kbits/s", true, SAMPLE_AVG);

	if(this->satellite_type == REGENERATIVE)
	{
		this->probe_used_modcod = Output::registerProbe<int>("ACM.Used_modcod",
		                                                     "modcod index",
		                                                     true, SAMPLE_LAST);
	}

	return true;
}

bool BlockDvbNcc::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			// first handle specific messages
			if(((MessageEvent *)event)->getMessageType() == msg_sig)
			{
				DvbFrame *frame = (DvbFrame *)((MessageEvent *)event)->getData();
				if(!this->handleDvbFrame(frame))
				{
					return false;
				}
				break;
			}
			else if(((MessageEvent *)event)->getMessageType() == msg_saloha)
			{
				list<DvbFrame *> *ack_frames;
				list<DvbFrame *>::iterator ack_it;
				ack_frames = (list<DvbFrame *> *)((MessageEvent *)event)->getData();
				for(ack_it = ack_frames->begin(); ack_it != ack_frames->end();
				    ++ack_it)
				{
					this->complete_dvb_frames.push_back(*ack_it);
				}
				delete ack_frames;
				break;
			}

			NetBurst *burst;
			NetBurst::iterator pkt_it;

			burst = (NetBurst *)((MessageEvent *)event)->getData();

			LOG(this->log_receive, LEVEL_INFO,
			    "SF#%u: encapsulation burst received "
			    "(%d packet(s))\n", this->super_frame_counter,
			    burst->length());

			// set each packet of the burst in MAC FIFO
			for(pkt_it = burst->begin(); pkt_it != burst->end(); ++pkt_it)
			{
				qos_t fifo_priority = (*pkt_it)->getQos();
				LOG(this->log_receive, LEVEL_INFO,
				    "SF#%u: store one encapsulation "
				    "packet\n", this->super_frame_counter);

				// find the FIFO associated to the IP QoS (= MAC FIFO id)
				// else use the default id
				if(this->dvb_fifos.find(fifo_priority) == this->dvb_fifos.end())
				{
					fifo_priority = this->default_fifo_id;
				}

				if(!this->onRcvEncapPacket(*pkt_it,
				                           this->dvb_fifos[fifo_priority],
				                           0))
				{
					// a problem occured, we got memory allocation error
					// or fifo full and we won't empty fifo until next
					// call to onDownwardEvent => return
					LOG(this->log_receive, LEVEL_ERROR,
					    "SF#%u: unable to store received "
					    "encapsulation packet (see previous errors)\n",
					    this->super_frame_counter);
					burst->clear();
					delete burst;
					return false;
				}

				LOG(this->log_receive, LEVEL_INFO,
				    "SF#%u: encapsulation packet is "
				    "successfully stored\n",
				    this->super_frame_counter);
			}
			burst->clear(); // avoid deteleting packets when deleting burst
			delete burst;
		}
		break;

		case evt_timer:
			// receive the frame Timer event
			LOG(this->log_receive, LEVEL_DEBUG,
			    "timer event received on downward channel");
			if(*event == this->frame_timer)
			{
				if(this->probe_frame_interval->isEnabled())
				{
					timeval time = event->getAndSetCustomTime();
					float val = time.tv_sec * 1000000L + time.tv_usec;
					this->probe_frame_interval->put(val/1000);
				}

				// we reached the end of a superframe
				// beginning of a new one, send SOF and run allocation
				// algorithms (DAMA)
				// increase the superframe number and reset
				// counter of frames per superframe
				this->super_frame_counter++;

				// send Start Of Frame (SOF)
				this->sendSOF();

				if(!this->dama_ctrl)
				{
					// stop here
					return true;
				}

				if(this->with_phy_layer)
				{
					// for each terminal in DamaCtrl update FMT because in this case
					// this it not done with scenario timer and FMT is updated
					// each received frame but we only need it for allocation
					this->dama_ctrl->updateFmt();
				}

				// run the allocation algorithms (DAMA)
				this->dama_ctrl->runOnSuperFrameChange(this->super_frame_counter);

				// send TTP computed by DAMA
				this->sendTTP();
			}
			else if(*event == this->fwd_timer)
			{
				uint32_t remaining_alloc_sym = 0;

				this->updateStats();
				this->fwd_frame_counter++;

				// schedule encapsulation packets
				// TODO loop on categories (see todo in initMode)
				// TODO In regenerative mode we should schedule in frame_timer ??
				if(!this->scheduling->schedule(this->fwd_frame_counter,
				                               this->getCurrentTime(),
				                               &this->complete_dvb_frames,
				                               remaining_alloc_sym))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "failed to schedule encapsulation "
					    "packets stored in DVB FIFO\n");
					return false;
				}
				if(this->satellite_type == REGENERATIVE &&
				   this->complete_dvb_frames.size() > 0)
				{
					// we can do that because we have only one MODCOD per allocation
					// TODO THIS IS NOT TRUE ! we schedule for each carriers, if
					// desired modcod is low we can send on many carriers
					uint8_t modcod_id;
					modcod_id =
						((DvbRcsFrame *)this->complete_dvb_frames.front())->getModcodId();
					this->probe_used_modcod->put(modcod_id);
				}
				LOG(this->log_receive, LEVEL_INFO,
				    "SF#%u: %u symbols remaining after "
				    "scheduling\n", this->super_frame_counter,
				    remaining_alloc_sym);
				if(!this->sendBursts(&this->complete_dvb_frames,
				                     this->data_carrier_id))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "failed to build and send DVB/BB "
					    "frames\n");
					return false;
				}
			}
			else if(*event == this->scenario_timer)
			{
				// if regenerative satellite and physical layer scenario,
				// send ACM parameters
				if(this->satellite_type == REGENERATIVE &&
				   this->with_phy_layer)
				{
					this->sendAcmParameters();
				}

				// it's time to update MODCOD IDs
				LOG(this->log_receive, LEVEL_DEBUG,
				    "MODCOD scenario timer received\n");

				if(!this->up_ret_fmt_simu.goNextScenarioStep(false) ||
				    !this->down_fwd_fmt_simu.goNextScenarioStep(true))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "SF#%u: failed to update MODCOD IDs\n",
					    this->super_frame_counter);
				}
				else
				{
					LOG(this->log_receive, LEVEL_DEBUG,
					    "SF#%u: MODCOD IDs successfully updated\n",
					    this->super_frame_counter);
				}
				if(this->dama_ctrl)
				{
					// TODO FMT in slotted aloha should be handled on ST
					//  => so remove return fmt simu !
					//  => keep this todo in order to think of it on ST

					// for each terminal in DamaCtrl update FMT
					this->dama_ctrl->updateFmt();
				}
			}
			else if(*event == this->simu_timer)
			{
				switch(this->simulate)
				{
					case file_simu:
						if(!this->simulateFile())
						{
							LOG(this->log_request_simulation, LEVEL_ERROR,
							    "file simulation failed");
							fclose(this->simu_file);
							this->simu_file = NULL;
							this->simulate = none_simu;
							this->removeEvent(this->simu_timer);
						}
						break;
					case random_simu:
						this->simulateRandom();
						break;
					default:
						break;
				}
				// flush files
				fflush(this->event_file);
			}
			else if(*event == this->pep_cmd_apply_timer)
			{
				// it is time to apply the command sent by the external
				// PEP component

				PepRequest *pep_request;

				LOG(this->log_receive, LEVEL_NOTICE,
				    "apply PEP requests now\n");
				while((pep_request = this->getNextPepRequest()) != NULL)
				{
					if(this->dama_ctrl->applyPepCommand(pep_request))
					{
						LOG(this->log_receive, LEVEL_NOTICE,
						    "PEP request successfully "
						    "applied in DAMA\n");
					}
					else
					{
						LOG(this->log_receive, LEVEL_ERROR,
						    "failed to apply PEP request "
						    "in DAMA\n");
						return false;
					}
				}
			}
			else
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "unknown timer event received %s\n",
				    event->getName().c_str());
				return false;
			}
			break;

		case evt_net_socket:
			if(*event == this->getPepListenSocket())
			{
				int ret;

				// event received on PEP listen socket
				LOG(this->log_receive, LEVEL_NOTICE,
				    "event received on PEP listen socket\n");

				// create the client socket to receive messages
				ret = acceptPepConnection();
				if(ret == 0)
				{
					LOG(this->log_receive, LEVEL_NOTICE,
					    "NCC is now connected to PEP\n");
					// add a fd to handle events on the client socket
					this->addNetSocketEvent("pep_client",
					                        this->getPepClientSocket(),
					                        200);
				}
				else if(ret == -1)
				{
					LOG(this->log_receive, LEVEL_WARNING,
					    "failed to accept new connection "
					    "request from PEP\n");
				}
				else if(ret == -2)
				{
					LOG(this->log_receive, LEVEL_WARNING,
					    "one PEP already connected: "
					    "reject new connection request\n");
				}
				else
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "unknown status %d from "
					    "acceptPepConnection()\n", ret);
					return false;
				}
			}
			else if(*event == this->getPepClientSocket())
			{
				// event received on PEP client socket
				LOG(this->log_receive, LEVEL_NOTICE,
				    "event received on PEP client socket\n");

				// read the message sent by PEP or delete socket
				// if connection is dead
				if(this->readPepMessage((NetSocketEvent *)event) == true)
				{
					// we have received a set of commands from the
					// PEP component, let's apply the resources
					// allocations/releases they contain

					// set delay for applying the commands
					if(this->getPepRequestType() == PEP_REQUEST_ALLOCATION)
					{
						if(!this->startTimer(this->pep_cmd_apply_timer))
						{
							LOG(this->log_receive, LEVEL_ERROR,
							    "cannot start pep timer");
							return false;
						}
						LOG(this->log_receive, LEVEL_NOTICE,
						    "PEP Allocation request, apply a %dms"
						    " delay\n", pep_alloc_delay);
					}
					else if(this->getPepRequestType() == PEP_REQUEST_RELEASE)
					{
						this->raiseTimer(this->pep_cmd_apply_timer);
						LOG(this->log_receive, LEVEL_NOTICE,
						    "PEP Release request, no delay to "
						    "apply\n");
					}
					else
					{
						LOG(this->log_receive, LEVEL_ERROR,
						    "cannot determine request type!\n");
						return false;
					}
				}
				else
				{
					LOG(this->log_receive, LEVEL_WARNING,
					    "network problem encountered with PEP, "
					    "connection was therefore closed\n");
					this->removeEvent(this->pep_cmd_apply_timer);
					return false;
				}
			}


		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event received %s",
			    event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockDvbNcc::Downward::handleDvbFrame(DvbFrame *dvb_frame)
{
	uint8_t msg_type = dvb_frame->getMessageType();
	switch(msg_type)
	{
		case MSG_TYPE_BBFRAME:
		case MSG_TYPE_DVB_BURST:
		case MSG_TYPE_CORRUPTED:
		{
			double curr_cni = dvb_frame->getCn();
			if(this->satellite_type == REGENERATIVE)
			{
				// regenerative case : we need downlink ACM parameters to inform
				//                     satellite with a SAC so inform opposite channel
				this->cni = curr_cni;
			}
			else
			{
				// transparent case : update return modcod for terminal
				DvbRcsFrame *frame = dvb_frame->operator DvbRcsFrame*();
				tal_id_t tal_id;
				// decode the first packet in frame to be able to get source terminal ID
				if(!this->up_return_pkt_hdl->getSrc(frame->getPayload(), tal_id))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "unable to read source terminal ID in"
					    " frame, won't be able to update C/N"
					    " value\n");
				}
				else
				{
					this->up_ret_fmt_simu.setRequiredModcod(tal_id, curr_cni);
				}
			}
		}
		break;

		case MSG_TYPE_SAC: // when physical layer is enabled
		{
			// TODO Sac *sac = dynamic_cast<Sac *>(dvb_frame);
			Sac *sac = (Sac *)dvb_frame;

			LOG(this->log_receive, LEVEL_DEBUG,
			    "handle received SAC\n");

			if(!this->dama_ctrl->hereIsSAC(sac))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to handle SAC frame\n");
				delete dvb_frame;
				goto error;
			}

			if(this->with_phy_layer)
			{
				// transparent : the C/N0 of forward link
				// regenerative : the C/N0 of uplink (updated by sat)
				double cni = sac->getCni();
				tal_id_t tal_id = sac->getTerminalId();
				if(this->satellite_type == TRANSPARENT)
				{
					this->down_fwd_fmt_simu.setRequiredModcod(tal_id, cni);
				}
				else
				{
					this->up_ret_fmt_simu.setRequiredModcod(tal_id, cni);
				}
			}
		}
		break;

		case MSG_TYPE_SESSION_LOGON_REQ:
			if(!this->handleLogonReq(dvb_frame))
			{
				goto error;
			}
			break;

		case MSG_TYPE_SESSION_LOGOFF:
			if(!this->handleLogoffReq(dvb_frame))
			{
				goto error;
			}
			break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "SF#%u: unknown type of DVB frame (%u), ignore\n",
			    this->super_frame_counter,
			    dvb_frame->getMessageType());
			delete dvb_frame;
			goto error;
	}

	delete dvb_frame;
	return true;

error:
	LOG(this->log_receive, LEVEL_ERROR,
	    "Treatments failed at SF#%u\n",
	    this->super_frame_counter);
	return false;
}


void BlockDvbNcc::Downward::sendSOF(void)
{
	Sof *sof = new Sof(this->super_frame_counter);

	// Send it
	if(!this->sendDvbFrame((DvbFrame *)sof, this->sof_carrier_id))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed to call sendDvbFrame() for SOF\n");
		return;
	}

	LOG(this->log_send, LEVEL_DEBUG,
	    "SF#%u: SOF sent\n", this->super_frame_counter);
}



bool BlockDvbNcc::Downward::handleLogonReq(DvbFrame *dvb_frame)
{
	LogonResponse *logon_resp;
	//TODO find why dynamic cast fail here and each time we do that on frames !?
//	LogonRequest *logon_req = dynamic_cast<LogonRequest *>(dvb_frame);
	LogonRequest *logon_req = (LogonRequest *)dvb_frame;
	uint16_t mac = logon_req->getMac();

	// handle ST for FMT simulation
	if(!this->up_ret_fmt_simu.doTerminalExist(mac) &&
	   !this->down_fwd_fmt_simu.doTerminalExist(mac))
	{
		// ST was not registered yet
		if(this->column_list.find(mac) == this->column_list.end() ||
		   !this->up_ret_fmt_simu.addTerminal(mac, this->column_list[mac]) ||
		   !this->down_fwd_fmt_simu.addTerminal(mac, this->column_list[mac]))
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "failed to handle FMT for ST %u, won't send logon response\n",
			    mac);
			goto release;
		}
	}

	// Inform the Dama controller (for its own context)
	if(this->dama_ctrl && !this->dama_ctrl->hereIsLogon(logon_req))
	{
		goto release;
	}
	logon_resp = new LogonResponse(mac, 0, mac);

	// Send it
	if(!this->sendDvbFrame((DvbFrame *)logon_resp,
	                       this->ctrl_carrier_id))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "Failed send logon response\n");
		goto release;
	}

	// send the corresponding event
	Output::sendEvent(this->event_logon_resp, "Logon response send to %u",
	                  mac);

	LOG(this->log_send, LEVEL_DEBUG,
	    "SF#%u: logon response sent to lower layer\n",
	    this->super_frame_counter);

	return true;

release:
	delete dvb_frame;
	return false;
}


bool BlockDvbNcc::Downward::handleLogoffReq(DvbFrame *dvb_frame)
{
	// TODO	Logoff *logoff = dynamic_cast<Logoff *>(dvb_frame);
	Logoff *logoff = (Logoff *)dvb_frame;

	// unregister the ST identified by the MAC ID found in DVB frame
	if(!this->up_ret_fmt_simu.delTerminal(logoff->getMac()) ||
	   !this->down_fwd_fmt_simu.delTerminal(logoff->getMac()))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to delete the ST with ID %d from FMT simulation\n",
		    logoff->getMac());
		delete dvb_frame;
		return false;
	}

	if(this->dama_ctrl)
	{
		this->dama_ctrl->hereIsLogoff(logoff);
	}
	LOG(this->log_receive, LEVEL_DEBUG,
	    "SF#%u: logoff request from %d\n",
	    this->super_frame_counter, logoff->getMac());

	delete dvb_frame;
	return true;
}


void BlockDvbNcc::Downward::sendTTP(void)
{
	Ttp *ttp = new Ttp(0, this->super_frame_counter);
	// Build TTP
	if(!this->dama_ctrl->buildTTP(ttp))
	{
		delete ttp;
		LOG(this->log_send, LEVEL_DEBUG,
		    "Dama didn't build TTP\bn");
		return;
	};

	if(!this->sendDvbFrame((DvbFrame *)ttp, this->ctrl_carrier_id))
	{
		delete ttp;
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed to send TTP\n");
		return;
	}

	LOG(this->log_send, LEVEL_DEBUG,
	    "SF#%u: TTP sent\n", this->super_frame_counter);
}

// TODO create a class for simulation and subclass file/random
bool BlockDvbNcc::Downward::simulateFile(void)
{
	enum
	{ none, cr, logon, logoff } event_selected;

	int resul;
	time_sf_t sf_nr;
	tal_id_t st_id;
	uint32_t st_request;
	rate_kbps_t st_rt;
	rate_kbps_t st_rbdc;
	vol_kb_t st_vbdc;
	int cr_type;

	if(this->simu_eof)
	{
		LOG(this->log_request_simulation, LEVEL_DEBUG,
		    "End of file\n");
		goto end;
	}

	sf_nr = 0;
	while(sf_nr <= this->super_frame_counter)
	{
		if(4 ==
		   sscanf(this->simu_buffer,
		          "SF%hu CR st%hu cr=%u type=%d",
		          &sf_nr, &st_id, &st_request, &cr_type))
		{
			event_selected = cr;
		}
		else if(5 ==
		        sscanf(this->simu_buffer,
		               "SF%hu LOGON st%hu rt=%hu rbdc=%hu vbdc=%hu",
		               &sf_nr, &st_id, &st_rt, &st_rbdc, &st_vbdc))
		{
			event_selected = logon;
		}
		else if(2 ==
		        sscanf(this->simu_buffer, "SF%hu LOGOFF st%hu", &sf_nr, &st_id))
		{
			event_selected = logoff;
		}
		else
		{
			event_selected = none;
		}
		if(event_selected != none && st_id <= BROADCAST_TAL_ID)
		{
			LOG(this->log_request_simulation, LEVEL_WARNING,
			    "Simulated ST%u ignored, IDs smaller than %u "
			    "reserved for emulated terminals\n",
			    st_id, BROADCAST_TAL_ID);
			goto loop_step;
		}
		if(event_selected == none)
			goto loop_step;
		if(sf_nr < this->super_frame_counter)
			goto loop_step;
		if(sf_nr > this->super_frame_counter)
			break;
		switch (event_selected)
		{
		case cr:
		{
			Sac *sac = new Sac(st_id);

			sac->addRequest(0, cr_type, st_request);
			LOG(this->log_request_simulation, LEVEL_INFO,
			    "SF#%u: send a simulated CR of type %u with "
			    "value = %u for ST %hu\n",
			    this->super_frame_counter, cr_type,
			    st_request, st_id);
			if(!this->dama_ctrl->hereIsSAC(sac))
			{
				goto error;
			}
			break;
		}
		case logon:
		{
			LogonRequest *sim_logon_req = new LogonRequest(st_id,
			                                               st_rt,
			                                               st_rbdc,
			                                               st_vbdc);
			bool ret = false;

			LOG(this->log_request_simulation, LEVEL_INFO,
			    "SF#%u: send a simulated logon for ST %d\n",
			    this->super_frame_counter, st_id);
			// check for column in FMT simulation list
			if(this->column_list.find(st_id) == this->column_list.end())
			{
				LOG(this->log_request_simulation, LEVEL_NOTICE,
				    "no column ID for simulated terminal, use the"
				    " terminal ID\n");
				ret = this->up_ret_fmt_simu.addTerminal(st_id, st_id) ||
				      this->down_fwd_fmt_simu.addTerminal(st_id, st_id);
			}
			else
			{
				ret = this->up_ret_fmt_simu.addTerminal(st_id, this->column_list[st_id]) ||
				      this->down_fwd_fmt_simu.addTerminal(st_id, this->column_list[st_id]);
			}
			if(!ret)
			{
				LOG(this->log_request_simulation, LEVEL_ERROR,
				    "failed to register simulated ST with MAC "
				    "ID %u\n", st_id);
				goto error;
			}

			if(!this->dama_ctrl->hereIsLogon(sim_logon_req))
			{
				goto error;
			}
		}
		break;
		case logoff:
		{
			Logoff *sim_logoff = new Logoff(st_id);
			LOG(this->log_request_simulation, LEVEL_INFO,
			    "SF#%u: send a simulated logoff for ST %d\n",
			    this->super_frame_counter, st_id);
			if(!this->dama_ctrl->hereIsLogoff(sim_logoff))
			{
				goto error;
			}
		}
		break;
		default:
			break;
		}
	 loop_step:
		resul = -1;
		while(resul < 1)
		{
			resul = fscanf(this->simu_file, "%254[^\n]\n", this->simu_buffer);
			if(resul == 0)
			{
				int ret;
				// No conversion occured, we simply skip the line
				ret = fscanf(this->simu_file, "%*s");
				if ((ret == 0) || (ret == EOF))
				{
					goto error;
				}
			}
			LOG(this->log_request_simulation, LEVEL_DEBUG,
			    "fscanf result=%d: %s", resul, this->simu_buffer);
			//fprintf (stderr, "frame %d\n", this->super_frame_counter);
			LOG(this->log_request_simulation, LEVEL_DEBUG,
			    "frame %u\n", this->super_frame_counter);
			if(resul == -1)
			{
				this->simu_eof = true;
				this->removeEvent(this->simu_timer);
				LOG(this->log_request_simulation, LEVEL_DEBUG,
				    "End of file.\n");
				goto end;
			}
		}
	}

end:
	return true;

 error:
	return false;
}


void BlockDvbNcc::Downward::simulateRandom(void)
{
	static bool initialized = false;

	int i;
	// BROADCAST_TAL_ID is maximum tal_id for emulated terminals
	tal_id_t sim_tal_id = BROADCAST_TAL_ID + 1;

	if(!initialized)
	{
		for(i = 0; i < this->simu_st; i++)
		{
			tal_id_t tal_id = sim_tal_id + i;
			LogonRequest *sim_logon_req = new LogonRequest(tal_id, this->simu_rt,
			                                               this->simu_max_rbdc,
			                                               this->simu_max_vbdc);
			bool ret = false;

			// check for column in FMT simulation list
			if(this->column_list.find(tal_id) == this->column_list.end())
			{
				LOG(this->log_request_simulation, LEVEL_NOTICE,
				    "no column ID for simulated terminal, use the"
				    " terminal ID\n");
				ret = this->up_ret_fmt_simu.addTerminal(tal_id, tal_id) ||
				      this->down_fwd_fmt_simu.addTerminal(tal_id, tal_id);
			}
			else
			{
				ret = this->up_ret_fmt_simu.addTerminal(tal_id, this->column_list[tal_id]) ||
				      this->down_fwd_fmt_simu.addTerminal(tal_id, this->column_list[tal_id]);
			}
			if(!ret)
			{
				LOG(this->log_request_simulation, LEVEL_ERROR,
				    "failed to register simulated ST with MAC"
				    " ID %u\n", tal_id);
				return;
			}

			this->dama_ctrl->hereIsLogon(sim_logon_req);
		}
		initialized = true;
	}

	for(i = 0; i < this->simu_st; i++)
	{
		uint32_t val;
		Sac *sac = new Sac(sim_tal_id + i);

		if(this->simu_interval)
		{
			val = this->simu_cr - this->simu_interval / 2 +
			      random() % this->simu_interval;
	    }
	    else
	    {
			val = this->simu_cr;
	    }
		sac->addRequest(0, access_dama_rbdc, val);

		this->dama_ctrl->hereIsSAC(sac);
	}
}

void BlockDvbNcc::Downward::updateStats(void)
{
	if(!this->doSendStats())
	{
		return;
	}
	// Update stats on the GW
	if(this->dama_ctrl)
	{
		this->dama_ctrl->updateStatistics(this->stats_period_ms);
	}

	mac_fifo_stat_context_t fifo_stat;
	// MAC fifos stats
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		(*it).second->getStatsCxt(fifo_stat);
		
		this->l2_to_sat_total_bytes += fifo_stat.out_length_bytes;

		this->probe_gw_l2_to_sat_before_sched[(*it).first]->put(
			fifo_stat.in_length_bytes * 8.0 / this->stats_period_ms);

		this->probe_gw_l2_to_sat_after_sched[(*it).first]->put(
			fifo_stat.out_length_bytes * 8.0 / this->stats_period_ms);

		// Mac fifo stats
		this->probe_gw_queue_size[(*it).first]->put(fifo_stat.current_pkt_nbr);
		this->probe_gw_queue_size_kb[(*it).first]->put(
					fifo_stat.current_length_bytes * 8 / 1000);
		this->probe_gw_queue_loss[(*it).first]->put(fifo_stat.drop_pkt_nbr);
		this->probe_gw_queue_loss_kb[(*it).first]->put(fifo_stat.drop_bytes * 8);
	}
	this->probe_gw_l2_to_sat_total->put(this->l2_to_sat_total_bytes * 8 /
	                                    this->stats_period_ms);
	
	//reset stat
	this->l2_to_sat_total_bytes = 0;
}
bool BlockDvbNcc::Downward::sendAcmParameters(void)
{
	Sac *send_sac = new Sac(GW_TAL_ID);
	send_sac->setAcm(this->cni);
	LOG(this->log_send, LEVEL_DEBUG,
	    "Send SAC with CNI = %.2f\n", this->cni);

	// Send message
	if(!this->sendDvbFrame((DvbFrame *)send_sac,
	                       this->ctrl_carrier_id))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "SF#%u: failed to send SAC\n",
		    this->super_frame_counter);
		delete send_sac;
		return false;
	}
	return true;
}


/*****************************************************************************/
/*                               Upward                                      */
/*****************************************************************************/

BlockDvbNcc::Upward::Upward(Block *const bl):
	DvbUpward(bl),
	saloha(NULL),
	mac_id(GW_TAL_ID),
	ret_fmt_groups(),
	scpc_pkt_hdl(NULL),
	probe_gw_l2_from_sat(NULL),
	probe_received_modcod(NULL),
	probe_rejected_modcod(NULL),
	log_saloha(NULL),
	event_logon_req(NULL)
{
}


BlockDvbNcc::Upward::~Upward()
{
	if(this->saloha)
		delete this->saloha;

	// delete FMT groups here because they may be present in many carriers
	// TODO do something to avoid groups here
	for(fmt_groups_t::iterator it = this->ret_fmt_groups.begin();
	    it != this->ret_fmt_groups.end(); ++it)
	{
		delete (*it).second;
	}
}


bool BlockDvbNcc::Upward::onInit(void)
{
	T_LINK_UP *link_is_up;
	string scheme;

	if(!this->initSatType())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize satellite type\n");
		goto error;
	}
	// get the common parameters
	if(this->satellite_type == TRANSPARENT)
	{
		scheme = RETURN_UP_ENCAP_SCHEME_LIST;
	}
	else
	{
		scheme = FORWARD_DOWN_ENCAP_SCHEME_LIST;
	}

	if(!this->initCommon(scheme.c_str()))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		goto error;
	}

	if(!this->initMode())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the mode part of the "
		    "initialisation\n");
		goto error;
	}

	// initialize the slotted Aloha part
	if(!this->initSlottedAloha())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the DAMA part of the "
		    "initialisation\n");
		goto error;
	}

	// synchronized with SoF
	this->initStatsTimer(this->ret_up_frame_duration_ms);

	if(!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialization of "
		    "statistics\n");
		goto error_mode;
	}

	// create and send a "link is up" message to upper layer
	link_is_up = new T_LINK_UP;
	if(link_is_up == NULL)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u: failed to allocate memory for link_is_up "
		    "message\n", this->super_frame_counter);
		goto error_mode;
	}
	link_is_up->group_id = 0;
	link_is_up->tal_id = GW_TAL_ID;

	if(!this->enqueueMessage((void **)(&link_is_up),
	                         sizeof(T_LINK_UP),
	                         msg_link_up))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u: failed to send link up message to upper layer",
		    this->super_frame_counter);
		delete link_is_up;
		goto error_mode;
	}
	LOG(this->log_init, LEVEL_DEBUG,
	    "SF#%u Link is up msg sent to upper layer\n",
	    this->super_frame_counter);

	// everything went fine
	return true;

error_mode:
	delete this->receptionStd;
	delete this->receptionStdScpc;
error:
	return false;
}

bool BlockDvbNcc::Upward::initSlottedAloha(void)
{
	TerminalCategories<TerminalCategorySaloha> sa_categories;
	TerminalMapping<TerminalCategorySaloha> sa_terminal_affectation;
	TerminalCategorySaloha *sa_default_category;
	int lan_scheme_nbr;

	// init fmt_simu
	if(!this->initModcodFiles(RETURN_UP_MODCOD_DEF_RCS,
	                          RETURN_UP_MODCOD_TIME_SERIES))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the up/return MODCOD files\n");
		return false;
	}

	if(!this->initBand<TerminalCategorySaloha>(RETURN_UP_BAND,
	                                           ALOHA,
	                                           this->ret_up_frame_duration_ms,
		                                       this->satellite_type,
	                                           this->fmt_simu.getModcodDefinitions(),
	                                           sa_categories,
	                                           sa_terminal_affectation,
	                                           &sa_default_category,
	                                           this->ret_fmt_groups))
	{
		return false;
	}

	// check if there is Slotted Aloha carriers
	if(sa_categories.size() == 0)
	{
		LOG(this->log_init, LEVEL_DEBUG,
		    "No Slotted Aloha carrier\n");
		return true;
	}

	// cannot use Slotted Aloha with regenerative satellite
	if(this->satellite_type == REGENERATIVE)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Carrier configured with Slotted Aloha while satellite "
		    "is regenerative\n");
		return false;
	}

	// TODO possible loss with Slotted Aloha and ROHC or MPEG
	//      (see TODO in TerminalContextSaloha.cpp)
	if(this->pkt_hdl->getName() == "MPEG2-TS")
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "Cannot guarantee no loss with MPEG2-TS and Slotted Aloha "
		    "on return link due to interleaving\n");
	}
	if(!Conf::getNbListItems(GLOBAL_SECTION, LAN_ADAPTATION_SCHEME_LIST,
	                         lan_scheme_nbr))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n", GLOBAL_SECTION,
		    LAN_ADAPTATION_SCHEME_LIST);
		return false;
	}
	for(int i = 0; i < lan_scheme_nbr; i++)
	{
		string name;
		if(!Conf::getValueInList(GLOBAL_SECTION, LAN_ADAPTATION_SCHEME_LIST,
		                         POSITION, toString(i), PROTO, name))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, invalid value %d for parameter '%s'\n",
			    GLOBAL_SECTION, i, POSITION);
			return false;
		}
		if(name == "ROHC")
		{
			LOG(this->log_init, LEVEL_WARNING,
			    "Cannot guarantee no loss with RoHC and Slotted Aloha "
			    "on return link due to interleaving\n");
		}
	}
	// end TODO

	// Create the Slotted ALoha part
	this->saloha = new SlottedAlohaNcc();
	if(!this->saloha)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to create Slotted Aloha\n");
		return false;
	}

	// Initialize the Slotted Aloha parent class
	// Unlike (future) scheduling, Slotted Aloha get all categories because
	// it also handles received frames and in order to know to which
	// category a frame is affected we need to get source terminal ID
	if(!this->saloha->initParent(this->ret_up_frame_duration_ms,
	                             // pkt_hdl is the up_ret one because transparent sat
	                             this->pkt_hdl))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Dama Controller Initialization failed.\n");
		goto release_saloha;
	}

	if(!this->saloha->init(sa_categories,
	                       sa_terminal_affectation,
	                       sa_default_category))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the DAMA controller\n");
		goto release_saloha;
	}

	return true;

release_saloha:
	delete this->saloha;
	return false;
}


bool BlockDvbNcc::Upward::initMode(void)
{
	// initialize the reception standard
	// depending on the satellite type
	if(this->satellite_type == TRANSPARENT)
	{
		this->receptionStd = new DvbRcsStd(this->pkt_hdl);

		if(!this->initPktHdl("GSE",
		                     &this->scpc_pkt_hdl, true))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to get packet handler for SCPC\n");
				goto error;
		}
	
		this->receptionStdScpc = new DvbScpcStd(this->scpc_pkt_hdl);
	}
	else if(this->satellite_type == REGENERATIVE)
	{
		this->receptionStd = new DvbS2Std(this->pkt_hdl);
	}
	else
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "unknown value '%u' for satellite type ",
		    this->satellite_type);
		goto error;

	}
	if(!this->receptionStd)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to create the reception standard\n");
		goto error;
	}

	if(this->satellite_type == TRANSPARENT && !this->receptionStdScpc)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to create the reception standard for SCPC\n");
		goto error;
	}


	return true;

error:
	return false;
}


bool BlockDvbNcc::Upward::initOutput(void)
{
	// Events
	this->event_logon_req = Output::registerEvent("DVB.logon_request");

	if(this->saloha)
	{
		this->log_saloha = Output::registerLog(LEVEL_WARNING, "Dvb.SlottedAloha");
	}

	// Output probes and stats
	this->probe_gw_l2_from_sat=
		Output::registerProbe<int>("Throughputs.L2_from_SAT",
		                           "Kbits/s", true, SAMPLE_AVG);
	this->l2_from_sat_bytes = 0;

	if(this->satellite_type == REGENERATIVE)
	{
		this->probe_received_modcod = Output::registerProbe<int>("ACM.Received_modcod",
		                                                         "modcod index",
		                                                         true, SAMPLE_LAST);
		this->probe_rejected_modcod = Output::registerProbe<int>("ACM.Rejected_modcod",
		                                                         "modcod index",
		                                                         true, SAMPLE_LAST);
	}
	return true;
}


bool BlockDvbNcc::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			if(!this->onRcvDvbFrame(dvb_frame))
			{
				return false;
			}
		}
		break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event received %s",
			    event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockDvbNcc::Upward::onRcvDvbFrame(DvbFrame *dvb_frame)
{
	uint8_t msg_type = dvb_frame->getMessageType();

	LOG(this->log_receive, LEVEL_INFO,
	    "DVB frame received with type %u\n", msg_type);
	switch(msg_type)
	{
		// burst
		case MSG_TYPE_BBFRAME:
		{
			// Ignore BB frames in transparent scenario
			// (this is required because the GW may receive BB frames
			//  in transparent scenario due to carrier emulation)
			
			//  TODO: This should be resolved with Multi-Spot (no need to test if BBframes is coming from GW)
			BBFrame *frame = dvb_frame->operator BBFrame*();
			tal_id_t tal_id;
			// decode the first packet in frame to be able to get source terminal ID
			if(!this->scpc_pkt_hdl->getSrc(frame->getPayload(), tal_id))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "unable to read source terminal ID in"
				    " frame, won't be able to discard BBframes sent from GW to GW\n");
			}
			else
			{
				if(tal_id == GW_TAL_ID)
				{
					LOG(this->log_receive, LEVEL_NOTICE,
					    "ignore received BB frame from GW in transparent "
					    "scenario\n");
					goto drop;
				}
			}

			NetBurst *burst = NULL;
			//DvbScpcStd *std = (DvbScpcStd *)this->receptionStdScpc;
			// Update stats
			this->l2_from_sat_bytes += dvb_frame->getMessageLength();
			this->l2_from_sat_bytes -= sizeof(T_DVB_HDR);
	
			if(this->with_phy_layer)
			{
				DvbFrame *frame_copy = new DvbFrame(dvb_frame);
				if(!this->shareFrame(frame_copy))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "Unable to transmit Frame to opposite channel\n");
				}
			}
			// GW_TAL_ID is no used
			if(!this->receptionStdScpc->onRcvFrame(dvb_frame,
		 	                                       GW_TAL_ID,
			                                       &burst))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to handle the reception of "
				    "BB frame (len = %u)\n",
				    dvb_frame->getMessageLength());
				goto error;
			}
			if(msg_type != MSG_TYPE_CORRUPTED)
			{
				// update MODCOD probes
				// TODO: for gateway
				//if(!this->with_phy_layer)
				//{
				//	this->probe_st_real_modcod->put(std->getRealModcod());
				//}
				//this->probe_received_modcod->put(std->getReceivedModcod());
				///TDOD: 28 for GW
				//uint8_t mc = 28;
				//this->probe_received_modcod->put(mc);
			}
			else
			{
				//TODO: 28 for GW
				//TODO see when FMT simulation will be improved
				//this->probe_rejected_modcod->put(std->getReceivedModcod());
				//uint8_t mc = 28;
				//this->probe_rejected_modcod->put(mc);
			}
			// send the message to the upper layer
			if(burst && !this->enqueueMessage((void **)&burst))
			{
				LOG(this->log_send, LEVEL_ERROR, 
				    "failed to send burst of packets to upper layer\n");
				delete burst;
				goto error;
			}
			LOG(this->log_send, LEVEL_INFO, 
			    "burst sent to the upper layer\n");
		}
		break;
						
			// breakthrough
		case MSG_TYPE_DVB_BURST:
		case MSG_TYPE_CORRUPTED:
		{
			NetBurst *burst = NULL;

			// Update stats
			this->l2_from_sat_bytes += dvb_frame->getPayloadLength();

			if(this->with_phy_layer)
			{
				DvbFrame *copy = new DvbFrame(dvb_frame);
				this->shareFrame(copy);
			}

			if(!this->receptionStd->onRcvFrame(dvb_frame,
			                                   this->mac_id,
			                                   &burst))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to handle DVB frame or BB frame\n");
				goto error;
			}
			if(this->receptionStd->getType() == "DVB-S2")
			{
				DvbS2Std *std = (DvbS2Std *)this->receptionStd;
				if(msg_type != MSG_TYPE_CORRUPTED)
				{
					this->probe_received_modcod->put(
							std->getReceivedModcod());
				}
				else
				{
					this->probe_rejected_modcod->put(
							std->getReceivedModcod());
				}
			}

			// send the message to the upper layer
			if(burst && !this->enqueueMessage((void **)&burst))
			{
				LOG(this->log_send, LEVEL_ERROR,
				    "failed to send burst of packets to upper layer\n");
				delete burst;
				goto error;
			}
			LOG(this->log_send, LEVEL_INFO,
			    "burst sent to the upper layer\n");
		}
		break;

		case MSG_TYPE_SAC:
			if(!this->shareFrame(dvb_frame))
			{
				goto error;
			}
			break;

		case MSG_TYPE_SESSION_LOGON_REQ:
			LOG(this->log_receive, LEVEL_INFO,
			    "Logon Req\n");
			if(!this->onRcvLogonReq(dvb_frame))
			{
				goto error;
			}
			break;

		case MSG_TYPE_SESSION_LOGOFF:
			LOG(this->log_receive, LEVEL_INFO,
			    "Logoff Req\n");
			if(!this->onRcvLogoffReq(dvb_frame))
			{
				goto error;
			}
			break;

		case MSG_TYPE_TTP:
		case MSG_TYPE_SESSION_LOGON_RESP:
			// nothing to do in this case
			LOG(this->log_receive, LEVEL_DEBUG,
			    "ignore TTP, logon response or SOF frame "
			    "(type = %d)\n", dvb_frame->getMessageType());
			delete dvb_frame;
			break;

		case MSG_TYPE_SOF:
		{
			this->updateStats();
			// if Slotted Aloha is enabled handled Slotted Aloha scheduling here.
			if(this->saloha)
			{
				uint16_t sfn;
				// TODO Sof *sof = dynamic_cast<Sof *>(dvb_frame);
				Sof *sof = (Sof *)dvb_frame;

				sfn = sof->getSuperFrameNumber();

				list<DvbFrame *> *ack_frames = new list<DvbFrame *>();
				// increase the superframe number and reset
				// counter of frames per superframe
				this->super_frame_counter++;
				if(this->super_frame_counter != sfn)
				{
					LOG(this->log_receive, LEVEL_WARNING,
					    "superframe counter (%u) is not the same as in SoF (%u)\n",
					    this->super_frame_counter, sfn);
					this->super_frame_counter = sfn;
				}

				// Slotted Aloha
				NetBurst* sa_burst = NULL;
				if(!this->saloha->schedule(&sa_burst,
				                           *ack_frames,
				                           this->super_frame_counter))
				{
					LOG(this->log_saloha, LEVEL_ERROR,
					    "failed to schedule Slotted Aloha\n");
					delete ack_frames;
					return false;
				}
				if(sa_burst &&
				   !this->enqueueMessage((void **)&sa_burst))
				{
					LOG(this->log_saloha, LEVEL_ERROR,
					    "Failed to send encapsulation packets to upper layer\n");
					delete ack_frames;
					return false;
				}
				if(ack_frames->size() &&
				   !this->shareMessage((void **)&ack_frames,
				                       sizeof(ack_frames),
				                       msg_saloha))
				{
					LOG(this->log_saloha, LEVEL_ERROR,
					    "Failed to send Slotted Aloha acks to opposite layer\n");
					delete ack_frames;
					return false;
				}
				// delete ack_frames if they are emtpy, else shareMessage
				// would set ack_frames == NULL
				if(ack_frames)
				{
					delete ack_frames;
				}
			}
			else
			{
				// nothing to do in this case
				LOG(this->log_receive, LEVEL_DEBUG,
				    "ignore SOF frame (type = %d)\n",
				    dvb_frame->getMessageType());
			}
			delete dvb_frame;
		}
		break;

		// Slotted Aloha
		case MSG_TYPE_SALOHA_DATA:
			// Update stats
			this->l2_from_sat_bytes += dvb_frame->getPayloadLength();

			if(!this->saloha->onRcvFrame(dvb_frame))
			{
				LOG(this->log_saloha, LEVEL_ERROR,
				    "failed to handle Slotted Aloha frame\n");
				goto error;
			}
			break;

		case MSG_TYPE_SALOHA_CTRL:
			delete dvb_frame;
			break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown type (%d) of DVB frame\n",
			    dvb_frame->getMessageType());
			delete dvb_frame;
			goto error;
			break;
	}

	return true;

error:
	LOG(this->log_receive, LEVEL_ERROR,
	    "Treatments failed at SF#%u\n",
	    this->super_frame_counter);
	return false;

drop:
   delete dvb_frame;
   return true;

}


bool BlockDvbNcc::Upward::onRcvLogonReq(DvbFrame *dvb_frame)
{
	//TODO find why dynamic cast fail here !?
//	LogonRequest *logon_req = dynamic_cast<LogonRequest *>(dvb_frame);
	LogonRequest *logon_req = (LogonRequest *)dvb_frame;
	uint16_t mac = logon_req->getMac();

	LOG(this->log_receive, LEVEL_INFO,
	    "Logon request from ST%u\n", mac);

	// refuse to register a ST with same MAC ID as the NCC
	if(mac == this->mac_id)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "a ST wants to register with the MAC ID of the NCC "
		    "(%d), reject its request!\n", mac);
		delete dvb_frame;
		return false;
	}

	// Inform SlottedAloha
	if(this->saloha)
	{
		if(!this->saloha->addTerminal(mac))
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "Cannot add terminal in Slotted Aloha context\n");
			return false;
		}
	}

	// send the corresponding event
	Output::sendEvent(this->event_logon_req, "Logon request received from %u",
	                  mac);

	// furnish response to opposite channel for sending
	if(!this->shareFrame(dvb_frame))
	{
		return false;
	}

	return true;
}


bool BlockDvbNcc::Upward::onRcvLogoffReq(DvbFrame *dvb_frame)
{
	if(!this->shareFrame(dvb_frame))
	{
		return false;
	}
	return true;
}

void BlockDvbNcc::Upward::updateStats(void)
{
	if(!this->doSendStats())
	{
		return;
	}
	this->probe_gw_l2_from_sat->put(
		this->l2_from_sat_bytes * 8.0 / this->stats_period_ms);
	this->l2_from_sat_bytes = 0;

	// Send probes
	Output::sendProbes();
}

bool BlockDvbNcc::Upward::shareFrame(DvbFrame *frame)
{
	if(!this->shareMessage((void **)&frame, sizeof(frame), msg_sig))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "Unable to transmit frame to opposite channel\n");
		delete frame;
		return false;
	}
	return true;
}

