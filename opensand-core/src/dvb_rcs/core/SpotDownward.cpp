/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 * Copyright © 2015 CNES
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
 * @file SpotDownward.cpp
 * @brief Downward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#include "SpotDownward.h"

#include "ForwardSchedulingS2.h"
#include "UplinkSchedulingRcs.h"
#include "DamaCtrlRcsLegacy.h"

#include <errno.h>


SpotDownward::SpotDownward(spot_id_t spot_id,
                           tal_id_t mac_id,
                           time_ms_t fwd_down_frame_duration,
                           time_ms_t ret_up_frame_duration,
                           time_ms_t stats_period,
                           sat_type_t sat_type,
                           EncapPlugin::EncapPacketHandler *pkt_hdl,
                           bool phy_layer):
	DvbChannel(),
	dama_ctrl(NULL),
	scheduling(NULL),
	fwd_frame_counter(0),
	ctrl_carrier_id(),
	sof_carrier_id(),
	data_carrier_id(),
	spot_id(spot_id),
	mac_id(mac_id),
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
	cni(100),
	column_list(),
	pep_cmd_apply_timer(),
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
	probe_gw_queue_size(),
	probe_gw_queue_size_kb(),
	probe_gw_queue_loss(),
	probe_gw_queue_loss_kb(),
	probe_gw_l2_to_sat_before_sched(),
	l2_to_sat_bytes_before_sched(),
	probe_gw_l2_to_sat_after_sched(),
	probe_gw_l2_to_sat_total(NULL),
	l2_to_sat_total_bytes(0),
	probe_frame_interval(NULL),
	probe_used_modcod(NULL),
	log_request_simulation(NULL),
	event_logon_resp(NULL)
{
	this->fwd_down_frame_duration_ms = fwd_down_frame_duration;
	this->ret_up_frame_duration_ms = ret_up_frame_duration;
	this->stats_period_ms = stats_period;
	this->satellite_type = sat_type;
	this->pkt_hdl = pkt_hdl;
	this->with_phy_layer = phy_layer;
}

SpotDownward::~SpotDownward()
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


bool SpotDownward::onInit(void)
{
	if(this->satellite_type == REGENERATIVE)
	{
		this->up_return_pkt_hdl = this->pkt_hdl;
	}
	else
	{
		if(!this->initPktHdl(RETURN_UP_ENCAP_SCHEME_LIST,
		                     &this->up_return_pkt_hdl))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "failed get packet handler\n");
			goto error;
		}
	}

	// Get the carrier Ids
	if(!this->initCarrierIds())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the carrier IDs part of the "
		    "initialisation\n");
		goto error;
	}

	if(!this->initFifo())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the FIFO part of the "
		    "initialisation\n");
		goto release_dama;
	}

	// Get and open the files
	if(!this->initModcodSimu())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the files part of the "
		    "initialisation\n");
		return false;
	}
	
	if(!this->initMode())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the mode part of the "
		    "initialisation\n");
		goto error;
	}

	// get and launch the dama algorithm
	if(!this->initDama())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the DAMA part of the "
		    "initialisation\n");
		goto error;
	}

	this->initStatsTimer(this->fwd_down_frame_duration_ms);

	// initialize the column ID for FMT simulation
	if(!this->initColumns())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the columns ID for FMT "
		    "simulation\n");
		goto release_dama;
	}

	if(!this->initRequestSimulation())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the request simulation part of "
		    "the initialisation\n");
		goto error;
	}

	if(!this->initOutput())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the initialization of "
		    "statistics\n");
		goto release_dama;
	}
	// everything went fine
	return true;

release_dama:
	delete this->dama_ctrl;
error:
	return false;
}

bool SpotDownward::initModcodSimu(void)
{
	if(!this->initModcodFiles(RETURN_UP_MODCOD_DEF_RCS,
	                          RETURN_UP_MODCOD_TIME_SERIES,
	                          this->up_ret_fmt_simu))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the up/return MODCOD files\n");
		return false;
	}
	if(!this->initModcodFiles(FORWARD_DOWN_MODCOD_DEF_S2,
	                          FORWARD_DOWN_MODCOD_TIME_SERIES,
	                          this->down_fwd_fmt_simu))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the forward MODCOD files\n");
		return false;
	}

	// initialize the MODCOD IDs
	if(!this->down_fwd_fmt_simu.goNextScenarioStep(true) ||
	   !this->up_ret_fmt_simu.goNextScenarioStep(false))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize MODCOD scheme IDs\n");
		return false;
	}

	return true;
}

// TODO completely rework columns and fmt_simu and take spots into account !!!
//      Moreover, for broadcast, in S2Scheduling, we get a MODCOD that can
//      be decoded by all STs, we should limit the STs to the one belonging
//      to the spot
bool SpotDownward::initColumns(void)
{
	int i = 0;
	ConfigurationList columns;
	ConfigurationList::iterator iter;

	// Get the list of STs
	if(!Conf::getListItems(Conf::section_map[SAT_SIMU_COL_SECTION],
		                   COLUMN_LIST, columns))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s, %s': problem retrieving simulation "
		    "column list\n", SAT_SIMU_COL_SECTION, COLUMN_LIST);
		goto error;
	}

	for(iter = columns.begin(); iter != columns.end(); ++iter)
	{
		i++;
		uint16_t tal_id;
		uint16_t column_nbr;

		// Get the Tal ID
		if(!Conf::getAttributeValue(iter, TAL_ID, tal_id))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "problem retrieving %s in simulation column "
			    "entry %d\n", TAL_ID, i);
			goto error;
		}
		// Get the column nbr
		if(!Conf::getAttributeValue(iter, COLUMN_NBR, column_nbr))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "problem retrieving %s in simulation column "
			    "entry %d\n", COLUMN_NBR, i);
			goto error;
		}

		this->column_list[tal_id] = column_nbr;
	}

	if(this->column_list.find(this->mac_id) == this->column_list.end())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "GW is not declared in column IDs\n");
		goto error;
	}

	// declare the GW as one ST for the MODCOD scenarios
	if(!this->up_ret_fmt_simu.addTerminal(this->mac_id,
	                                      this->column_list[this->mac_id]) ||
	   !this->down_fwd_fmt_simu.addTerminal(this->mac_id,
	                                        this->column_list[this->mac_id]))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to define the GW as ST with ID %d\n",
		    this->mac_id);
		goto error;
	}

	return true;

error:
	return false;
}


bool SpotDownward::initMode(void)
{
	TerminalCategoryDama *cat;

	// initialize scheduling
	// depending on the satellite type
	if(this->satellite_type == TRANSPARENT)
	{
		ConfigurationList forward_down_band = Conf::section_map[FORWARD_DOWN_BAND];
		ConfigurationList spots;
		ConfigurationList current_spot;
		ConfigurationList current_gw;

		if(!Conf::getListNode(forward_down_band, SPOT_LIST, spots))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "there is no %s into %s section\n",
			    SPOT_LIST, FORWARD_DOWN_BAND);
			return false;
		}

		if(!Conf::getElementWithAttributeValue(spots, ID,
		                                       this->spot_id, current_spot))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "there is no attribute %s with value: %d into %s/%s\n",
			    ID, this->spot_id, FORWARD_DOWN_BAND, SPOT_LIST);
			return false;
		}
		
		if(!Conf::getElementWithAttributeValue(current_spot, GW,
		                                       this->mac_id, current_gw))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "there is no attribute %s with value: %d into %s/%s\n",
			    ID, this->spot_id, FORWARD_DOWN_BAND, SPOT_LIST);
			return false;
		}
		if(!this->initBand<TerminalCategoryDama>(current_gw,
			                                     FORWARD_DOWN_BAND,
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
			// To implement more than one category we will need to create
			// one (a group of) fifo(s) per category and schedule per
			// (group of) fifo(s).
			// The packets would then be pushed in the correct (group of)
			// fifo(s) according to the category the destination
			// terminal ID belongs this is why we have categories,
			// terminal_affectation and default_category as attributes
			// map<cat label, sched> and fifos in scheduler ?
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot support more than one category for "
			    "down/forward band\n");
			return false;
		}

		cat = this->categories.begin()->second;
		this->scheduling = new ForwardSchedulingS2(this->fwd_down_frame_duration_ms,
		                                           this->pkt_hdl,
		                                           this->dvb_fifos,
		                                           &this->down_fwd_fmt_simu,
		                                           cat, this->spot_id,
		                                           true, this->mac_id);
	}
	else if(this->satellite_type == REGENERATIVE)
	{
		// get RETURN_UP_BAND section
		ConfigurationList return_up_band = Conf::section_map[RETURN_UP_BAND];
		ConfigurationList spots;
		ConfigurationList current_spot;
		ConfigurationList current_gw;

		// Get the spot list
		if(!Conf::getListNode(return_up_band, SPOT_LIST, spots))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "there is no %s into %s section\n",
			    SPOT_LIST, RETURN_UP_BAND);
			return false;
		}

		// get the spot which have the same id as SpotDownward
		if(!Conf::getElementWithAttributeValue(spots, ID,
		                                       this->spot_id, current_spot))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "there is no attribute %s with value: %d into %s/%s\n",
			    ID, this->spot_id, RETURN_UP_BAND, SPOT_LIST);
			return false;
		}

		if(!Conf::getElementWithAttributeValue(current_spot, GW,
		                                       this->mac_id, current_gw))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "there is no attribute %s with value: %d into %s/%s\n",
			    ID, this->spot_id, RETURN_UP_BAND, SPOT_LIST);
			return false;
		}
		if(!this->initBand<TerminalCategoryDama>(current_gw,
			                                     RETURN_UP_BAND,
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
		if(this->terminal_affectation.find(this->mac_id) != this->terminal_affectation.end())
		{
			cat = this->terminal_affectation[this->mac_id];
		}
		else
		{
			if(!this->default_category)
			{
				LOG(this->log_init_channel, LEVEL_ERROR,
				    "No default category and GW has no affectation\n");
				return false;
			}
			cat = this->default_category;
		}
		this->scheduling = new UplinkSchedulingRcs(this->pkt_hdl,
		                                           this->dvb_fifos,
		                                           &this->up_ret_fmt_simu,
		                                           cat,
		                                           this->mac_id);
	}
	else
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "unknown value '%u' for satellite type \n",
		    this->satellite_type);
		goto error;

	}
	if(!this->scheduling)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to create the scheduling\n");
		goto error;
	}

	return true;

error:
	return false;
}

bool SpotDownward::initCarrierIds(void)
{

	ConfigurationList carrier_list ;
	ConfigurationList spot_list;
	ConfigurationList::iterator iter;
	ConfigurationList::iterator iter_spots;
	ConfigurationList current_spot;
	ConfigurationList current_gw;

	if(!Conf::getListNode(Conf::section_map[SATCAR_SECTION], SPOT_LIST,
	                      spot_list))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s, %s': missing satellite channels\n",
		    SATCAR_SECTION, SPOT_LIST);
		goto error;
	}

	if(!Conf::getElementWithAttributeValue(spot_list, ID,
	                                       this->spot_id, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no attribute %s with value: %d into %s\n",
		    ID, this->spot_id, SPOT_LIST);
		goto error;
	}
	
	if(!Conf::getElementWithAttributeValue(current_spot, GW,
	                                       this->mac_id, current_gw))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no attribute %s with value: %d into %s %d\n",
		    GW, this->mac_id, SPOT_LIST, this->spot_id);
		goto error;
	}
	
	// TODO can we get current_spot with only one command ?
	//  -> get(section_map, SPOT_LIST, ID, id, current_spot) ?

	// get satellite channels from configuration
	if(!Conf::getListItems(current_gw, CARRIER_LIST, carrier_list))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s, %s': missing satellite channels\n",
		    SATCAR_SECTION, CARRIER_LIST);
		goto error;
	}

	for(iter = carrier_list.begin(); iter != carrier_list.end(); ++iter)
	{
		unsigned int carrier_id;
		string carrier_type;
		// Get the carrier id
		if(!Conf::getAttributeValue(iter, CARRIER_ID, carrier_id))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "section '%s/%s%d/%s': missing parameter '%s'\n",
			    SATCAR_SECTION, SPOT_LIST, this->spot_id,
			    CARRIER_LIST, CARRIER_ID);
			goto error;
		}

		// Get the carrier type
		if(!Conf::getAttributeValue(iter, CARRIER_TYPE, carrier_type))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "section '%s/%s%d/%s': missing parameter '%s'\n",
			    SATCAR_SECTION, SPOT_LIST, this->spot_id,
			    CARRIER_LIST, CARRIER_TYPE);
			goto error;
		}

		if(strcmp(carrier_type.c_str(), CTRL_IN)==0)
		{
			this->ctrl_carrier_id = carrier_id;
			this->sof_carrier_id = carrier_id;
		}
		else if(strcmp(carrier_type.c_str(), DATA_IN_GW)==0)
		{
			this->data_carrier_id = carrier_id;
		}
	}

	// Check carrier errors

	// Control carrier error
	if(this->ctrl_carrier_id == 0)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "SF#%u %s missing from section %s/%s%d\n",
		    this->super_frame_counter,
		    DVB_CAR_ID_CTRL, SATCAR_SECTION,
		    SPOT_LIST, this->spot_id);
		goto error;
	}

	// Logon carrier error
	if(this->sof_carrier_id == 0)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "SF#%u %s missing from section %s/%s%d\n",
		    this->super_frame_counter,
		    DVB_SOF_CAR, SATCAR_SECTION,
		    SPOT_LIST, this->spot_id);
		goto error;
	}

	// Data carrier error
	if(this->data_carrier_id == 0)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "SF#%u %s missing from section %s/%s%d\n",
		    this->super_frame_counter,
		    DVB_CAR_ID_DATA, SATCAR_SECTION,
		    SPOT_LIST, this->spot_id);
		goto error;
	}

	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "SF#%u: carrier IDs for Ctrl = %u, Sof = %u, "
	    "Data = %u\n", this->super_frame_counter,
	    this->ctrl_carrier_id,
	    this->sof_carrier_id, this->data_carrier_id);

	return true;

error:
	return false;
}

// TODO this function is NCC part but other functions are related to GW,
//      we could maybe create two classes inside the block to keep them separated
bool SpotDownward::initDama(void)
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
	if(!Conf::getValue(Conf::section_map[DC_SECTION_NCC],
		               DC_CRA_DECREASE, cra_decrease))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "missing %s parameter\n", DC_CRA_DECREASE);
		goto error;
	}
	LOG(this->log_init_channel, LEVEL_NOTICE, "cra_decrease = %s\n",
	    cra_decrease == true ? "true" : "false");

	// Retrieving the free capacity assignement parameter
	if(!Conf::getValue(Conf::section_map[DC_SECTION_NCC],
		               DC_FREE_CAP, fca_kbps))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "missing %s parameter\n", DC_FREE_CAP);
		goto error;
	}
	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "fca = %d kb/s\n", fca_kbps);

	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
		               SYNC_PERIOD, sync_period_ms))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Missing %s\n", SYNC_PERIOD);
		goto error;
	}
	sync_period_frame = (time_frame_t)round((double)sync_period_ms /
	                                        (double)this->ret_up_frame_duration_ms);
	rbdc_timeout_sf = sync_period_frame + 1;

	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "rbdc_timeout = %d superframes computed from sync period %d superframes\n",
	    rbdc_timeout_sf, sync_period_frame);

	if(this->satellite_type == TRANSPARENT)
	{
		ConfigurationList return_up_band = Conf::section_map[RETURN_UP_BAND];
		ConfigurationList spots;
		ConfigurationList current_spot;
		ConfigurationList current_gw;

		if(!Conf::getListNode(return_up_band, SPOT_LIST, spots))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "there is no %s into %s section",
			    SPOT_LIST, RETURN_UP_BAND);
			return false;
		}

		if(!Conf::getElementWithAttributeValue(spots, ID,
		                                       this->spot_id, current_spot))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "there is no attribute %s with value: %d into %s\n",
			    ID, this->spot_id, SPOT_LIST);
			return false;
		}

		if(!Conf::getElementWithAttributeValue(current_spot, GW,
		                                       this->mac_id, current_gw))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "there is no attribute %s with value: %d into %s\n",
			    ID, this->spot_id, SPOT_LIST);
			return false;
		}
		if(!this->initBand<TerminalCategoryDama>(current_gw,
			                                     RETURN_UP_BAND,
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
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "No DAMA and regenerative satellite\n");
			return false;
		}
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "No TDM carrier, won't allocate DAMA\n");
		// Also disable request simulation
		this->simulate = none_simu;
		return true;
	}

	// dama algorithm
	if(!Conf::getValue(Conf::section_map[DVB_NCC_SECTION],
		               DVB_NCC_DAMA_ALGO,
	                   dama_algo))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DVB_NCC_SECTION, DVB_NCC_DAMA_ALGO);
		goto error;
	}

	/* select the specified DAMA algorithm */
	if(dama_algo == "Legacy")
	{
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "creating Legacy DAMA controller\n");
		this->dama_ctrl = new DamaCtrlRcsLegacy(this->spot_id);
	}
	else
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s': bad value for parameter '%s'\n",
		    DVB_NCC_SECTION, DVB_NCC_DAMA_ALGO);
		goto error;
	}

	if(!this->dama_ctrl)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
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
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Dama Controller Initialization failed.\n");
		goto release_dama;
	}

	if(!this->dama_ctrl->init())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
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


bool SpotDownward::initFifo(void)
{
	ConfigurationList fifo_list;
	ConfigurationList::iterator iter;
	ConfigurationList spot_list;
	xmlpp::Node *spot_node = NULL;
	ConfigurationList::iterator iter_spots;

	/**********************************
	 *       Create SPOT_LIST
	 *********************************/
	// get satellite channels from configuration
	if(!Conf::getListNode(Conf::section_map[DVB_NCC_SECTION], SPOT_LIST,
	                      spot_list))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s, %s': missing satellite channels\n",
		    SATCAR_SECTION, SPOT_LIST);
		return false;;
	}
	// TODO function to directly get the correct spot with spot_id in order
	//      done in 2 functions above
	for(iter_spots = spot_list.begin(); iter_spots != spot_list.end();
	    ++iter_spots)
	{
		spot_id_t current_spot_id;
		if(!Conf::getAttributeValue(iter_spots, ID, current_spot_id))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "section %s/%s: missing attribute %s\n",
			    SATCAR_SECTION, SPOT_LIST, ID);
			return false;
		}

		//  check spot id to get good carriers!
		if(this->spot_id == current_spot_id)
		{
			spot_node = *iter_spots;
			// spot is found
			break;
		}
	}
	if(!spot_node)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "cannot found channels for spot %u\n", this->spot_id);
		return false;
	}
	// TODO why ?? overload functions in conf to avoid that

	/*
	 * Read the MAC queues configuration in the configuration file.
	 * Create and initialize MAC FIFOs
	 */
	if(!Conf::getListItems(spot_node,
	                       FIFO_LIST, fifo_list))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s, %s': missing fifo list\n", DVB_NCC_SECTION,
		    FIFO_LIST);
		goto err_fifo_release;
	}

	for(iter = fifo_list.begin(); iter != fifo_list.end(); ++iter)
	{
		unsigned int fifo_priority;
		vol_pkt_t fifo_size = 0;
		string fifo_name;
		string fifo_access_type;
		DvbFifo *fifo;

		// get fifo_id --> fifo_priority
		if(!Conf::getAttributeValue(iter, FIFO_PRIO, fifo_priority))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_PRIO, DVB_NCC_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get fifo_name
		if(!Conf::getAttributeValue(iter, FIFO_NAME, fifo_name))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_NAME, DVB_NCC_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get fifo_size
		if(!Conf::getAttributeValue(iter, FIFO_SIZE, fifo_size))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_SIZE, DVB_NCC_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get the fifo CR type
		if(!Conf::getAttributeValue(iter, FIFO_ACCESS_TYPE,
		                            fifo_access_type))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_ACCESS_TYPE, DVB_NCC_SECTION,
			    FIFO_LIST);
			goto err_fifo_release;
		}

		fifo = new DvbFifo(fifo_priority, fifo_name,
		                   fifo_access_type, fifo_size);

		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "Fifo priority = %u, FIFO name %s, size %u, "
		    "access type %d\n",
		    fifo->getPriority(),
		    fifo->getName().c_str(),
		    fifo->getMaxSize(),
		    fifo->getAccessType());

		// the default FIFO is the last one = the one with the smallest
		// priority actually, the IP plugin should add packets in the
		// default FIFO if the DSCP field is not recognize, default_fifo_id
		// should not be used this is only used if traffic categories
		// configuration and fifo configuration are not coherent.
		this->default_fifo_id = std::max(this->default_fifo_id,
		                                 fifo->getPriority());

		this->dvb_fifos.insert(pair<unsigned int, DvbFifo *>(fifo->getPriority(),
		                                                     fifo));
	} // end for(queues are now instanciated and initialized)

	this->resetStatsCxt();


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

bool SpotDownward::initOutput(void)
{
	// Events
	this->event_logon_resp = Output::registerEvent("Spot_%d.DVB.logon_response",
	                                               this->spot_id);

	// Logs
	if(this->simulate != none_simu)
	{
		this->log_request_simulation = Output::registerLog(LEVEL_WARNING,
		                                                   "Spot_%d,Dvb.RequestSimulation",
		                                                   this->spot_id);
	}

	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		const char *fifo_name = ((*it).second)->getName().data();
		unsigned int id = (*it).first;

		this->probe_gw_queue_size[id] =
			Output::registerProbe<int>("Packets", true, SAMPLE_LAST,
		                               "Spot_%d.Queue size.packets.%s",
		                               spot_id, fifo_name);
		this->probe_gw_queue_size_kb[id] =
			Output::registerProbe<int>("kbits", true, SAMPLE_LAST,
		                               "Spot_%d.Queue size.%s", spot_id, fifo_name);
		this->probe_gw_l2_to_sat_before_sched[id] =
			Output::registerProbe<int>("Kbits/s", true, SAMPLE_AVG,
		                               "Spot_%d.Throughputs.L2_to_SAT_before_sched.%s",
		                               spot_id, fifo_name);
		this->probe_gw_l2_to_sat_after_sched[id] =
			Output::registerProbe<int>("Kbits/s", true, SAMPLE_AVG,
		                               "Spot_%d.Throughputs.L2_to_SAT_after_sched.%s",
		                               spot_id, fifo_name);
		this->probe_gw_queue_loss[id] =
			Output::registerProbe<int>("Packets", true, SAMPLE_SUM,
		                               "Spot_%d.Queue loss.packets.%s",
		                               spot_id, fifo_name);
		this->probe_gw_queue_loss_kb[id] =
			Output::registerProbe<int>("Kbits/s", true, SAMPLE_SUM,
		                               "Spot_%d.Queue loss.%s",
		                               spot_id, fifo_name);
	}
	this->probe_gw_l2_to_sat_total =
		Output::registerProbe<int>("Kbits/s", true, SAMPLE_AVG,
		                           "Spot_%d.Throughputs.L2_to_SAT_after_sched.total",
		                           spot_id);

	if(this->satellite_type == REGENERATIVE)
	{
		this->probe_used_modcod = Output::registerProbe<int>("modcod index",
		                                                     true, SAMPLE_LAST,
		                                                     "Spot_%d.ACM.Used_modcod",
		                                                     this->spot_id);
	}

	return true;
}

bool SpotDownward::initRequestSimulation(void)
{

	ConfigurationList dvb_ncc_section = Conf::section_map[DVB_NCC_SECTION];
	ConfigurationList spots;
	ConfigurationList current_spot;

	if(!Conf::getListNode(dvb_ncc_section, SPOT_LIST, spots))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no %s into %s section\n",
		    SPOT_LIST, DVB_NCC_SECTION);
		return false;
	}

	if(!Conf::getElementWithAttributeValue(spots, ID,
		                                   this->spot_id, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no attribute %s with value: %d into %s\n",
		    ID, this->spot_id, SPOT_LIST);
		return false;
	}

	string str_config;

	memset(this->simu_buffer, '\0', SIMU_BUFF_LEN);
	// Get and open the event file
	if(!Conf::getValue(current_spot, DVB_EVENT_FILE, str_config))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "cannot load parameter %s from section %s\n",
		    DVB_EVENT_FILE, DVB_NCC_SECTION);
		return false;
	}
	if(str_config != "none" && this->with_phy_layer)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "cannot use simulated request with physical layer "
		    "because we need to add cni parameters in SAC (TBD!)\n");
		return false;
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
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "%s\n", strerror(errno));
		}
	}
	if(this->event_file == NULL && str_config != "none")
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "no record file will be used for event\n");
	}
	else if(this->event_file != NULL)
	{
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "events recorded in %s.\n", str_config.c_str());
	}

	// Get and set simulation parameter
	this->simulate = none_simu;
	if(!Conf::getValue(current_spot, DVB_SIMU_MODE, str_config))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "cannot load parameter %s from section %s\n",
		    DVB_SIMU_MODE, DVB_NCC_SECTION);
		return false;
	}

	// TODO for stdin use FileEvent for simu_timer ?
	if(str_config == "file")
	{
		if(!Conf::getValue(current_spot, DVB_SIMU_FILE, str_config))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot load parameter %s from section %s\n",
			    DVB_SIMU_FILE, DVB_NCC_SECTION);
			return false;
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
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "%s\n", strerror(errno));
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "no simulation file will be used.\n");
		}
		else
		{
			LOG(this->log_init_channel, LEVEL_NOTICE,
			    "events simulated from %s.\n",
			    str_config.c_str());
			this->simulate = file_simu;
		}
	}
	else if(str_config == "random")
	{
		int val;

		if(!Conf::getValue(current_spot, DVB_SIMU_RANDOM, str_config))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot load parameter %s from section %s\n",
			    DVB_SIMU_RANDOM, DVB_NCC_SECTION);
            return false;
		}
		val = sscanf(str_config.c_str(), "%ld:%ld:%ld:%ld:%ld:%ld",
		             &this->simu_st, &this->simu_rt, &this->simu_max_rbdc,
		             &this->simu_max_vbdc, &this->simu_cr, &this->simu_interval);
		if(val < 4)
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot load parameter %s from section %s\n",
			    DVB_SIMU_RANDOM, DVB_NCC_SECTION);
			return false;
		}
		else
		{
			LOG(this->log_init_channel, LEVEL_NOTICE,
			    "random events simulated for %ld terminals with "
			    "%ld kb/s bandwidth, %ld kb/s max RBDC, "
			    "%ld kb max VBDC, a mean request of %ld kb/s "
			    "and a request amplitude of %ld kb/s)i\n",
			    this->simu_st, this->simu_rt, this->simu_max_rbdc,
			    this->simu_max_vbdc, this->simu_cr,
			    this->simu_interval);
		}
		this->simulate = random_simu;
		srandom(times(NULL));
	}
	else
	{
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "no event simulation\n");
	}

	return true;
}


bool SpotDownward::handleSalohaAcks(const list<DvbFrame *> *ack_frames)
{
	list<DvbFrame *>::const_iterator ack_it;
	for(ack_it = ack_frames->begin(); ack_it != ack_frames->end();
	    ++ack_it)
	{
		this->complete_dvb_frames.push_back(*ack_it);
	}
	return true;
}

bool SpotDownward::handleEncapPacket(NetPacket *packet)
{
	qos_t fifo_priority = packet->getQos();
	LOG(this->log_receive_channel, LEVEL_INFO,
	    "SF#%u: store one encapsulation "
	    "packet\n", this->super_frame_counter);

	// find the FIFO associated to the IP QoS (= MAC FIFO id)
	// else use the default id
	if(this->dvb_fifos.find(fifo_priority) == this->dvb_fifos.end())
	{
		fifo_priority = this->default_fifo_id;
	}

	if(!this->pushInFifo(this->dvb_fifos[fifo_priority], packet, 0))
	{
		// a problem occured, we got memory allocation error
		// or fifo full and we won't empty fifo until next
		// call to onDownwardEvent => return
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "SF#%u: unable to store received "
		    "encapsulation packet (see previous errors)\n",
		    this->super_frame_counter);
		return false;
	}

	LOG(this->log_receive_channel, LEVEL_INFO,
	    "SF#%u: encapsulation packet is "
	    "successfully stored\n",
	    this->super_frame_counter);
	this->l2_to_sat_bytes_before_sched[fifo_priority] += packet->getTotalLength();

	return true;
}

bool SpotDownward::handleFwdFrameTimer(time_sf_t fwd_frame_counter)
{
	uint32_t remaining_alloc_sym = 0;
	this->fwd_frame_counter = fwd_frame_counter;
	this->updateStatistics();

	// schedule encapsulation packets
	// TODO loop on categories (see todo in initMode)
	// TODO In regenerative mode we should schedule in frame_timer ??
	if(!this->scheduling->schedule(this->fwd_frame_counter,
	                               getCurrentTime(),
	                               &this->complete_dvb_frames,
	                               remaining_alloc_sym))
	{
		LOG(this->log_send_channel, LEVEL_ERROR,
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
		modcod_id = ((DvbRcsFrame *)this->complete_dvb_frames.front())->getModcodId();
		this->probe_used_modcod->put(modcod_id);
	}
	LOG(this->log_receive_channel, LEVEL_INFO,
	    "SF#%u: %u symbols remaining after "
	    "scheduling\n", this->super_frame_counter,
	    remaining_alloc_sym);

	return true;
}


bool SpotDownward::handleLogonReq(const LogonRequest *logon_req)
{
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
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "failed to handle FMT for ST %u, "
			    "won't send logon response\n", mac);
			return false;
		}
	}

	// Inform the Dama controller (for its own context)
	if(this->dama_ctrl && !this->dama_ctrl->hereIsLogon(logon_req))
	{
		return false;
	}

	// send the corresponding event
	Output::sendEvent(this->event_logon_resp,
	                  "Logon response send to %u",
	                  mac);

	LOG(this->log_send_channel, LEVEL_DEBUG,
	    "SF#%u: logon response sent to lower layer\n",
	    this->super_frame_counter);

	return true;
}


bool SpotDownward::handleLogoffReq(const DvbFrame *dvb_frame)
{
	// TODO	Logoff *logoff = dynamic_cast<Logoff *>(dvb_frame);
	Logoff *logoff = (Logoff *)dvb_frame;

	// unregister the ST identified by the MAC ID found in DVB frame
	if(!this->up_ret_fmt_simu.delTerminal(logoff->getMac()) ||
	   !this->down_fwd_fmt_simu.delTerminal(logoff->getMac()))
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "failed to delete the ST with ID %d from FMT simulation\n",
		    logoff->getMac());
		delete dvb_frame;
		return false;
	}

	if(this->dama_ctrl)
	{
		this->dama_ctrl->hereIsLogoff(logoff);
	}
	LOG(this->log_receive_channel, LEVEL_DEBUG,
	    "SF#%u: logoff request from %d\n",
	    this->super_frame_counter, logoff->getMac());

	delete dvb_frame;
	return true;
}

bool SpotDownward::handleCorrutedFrame(DvbFrame *dvb_frame)
{
	double curr_cni = dvb_frame->getCn();
	if(this->satellite_type == REGENERATIVE)
	{
		// regenerative case:
		//   we need downlink ACM parameters to inform
		//   satellite with a SAC so inform opposite channel
		this->cni = curr_cni;
	}
	else
	{
		// transparent case : update return modcod for terminal
		DvbRcsFrame *frame = dvb_frame->operator DvbRcsFrame*();
		tal_id_t tal_id;
		// decode the first packet in frame to be able to
		// get source terminal ID
		if(!this->pkt_hdl->getSrc(frame->getPayload(),
					tal_id))
		{
			LOG(this->log_receive_channel, LEVEL_ERROR,
					"unable to read source terminal ID in"
					" frame, won't be able to update C/N"
					" vailue\n");
			return false;
		}
		else
		{
			this->up_ret_fmt_simu.setRequiredModcod(tal_id,
					curr_cni);
		}
	}
	return true;
}


bool SpotDownward::goNextScenarioStep()
{
	return !this->up_ret_fmt_simu.goNextScenarioStep(false) ||
					   !this->down_fwd_fmt_simu.goNextScenarioStep(true);
}

// TODO create a class for simulation and subclass file/random
bool SpotDownward::simulateFile(void)
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


bool SpotDownward::simulateRandom(void)
{
	static bool initialized = false;

	int i;
	// BROADCAST_TAL_ID is maximum tal_id for emulated terminals
	tal_id_t sim_tal_id = BROADCAST_TAL_ID + 1;

	if(!initialized)
	{
		// TODO function initRandomSimu
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
				ret = this->up_ret_fmt_simu.addTerminal(tal_id,
				                                        this->column_list[tal_id]) ||
				      this->down_fwd_fmt_simu.addTerminal(tal_id,
				                                          this->column_list[tal_id]);
			}
			if(!ret)
			{
				LOG(this->log_request_simulation, LEVEL_ERROR,
				    "failed to register simulated ST with MAC"
				    " ID %u\n", tal_id);
				return false;
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

		if(!this->dama_ctrl->hereIsSAC(sac))
		{
			return false;
		}
	}
	return true;
}

bool SpotDownward::buildTtp(Ttp *ttp)
{
	return this->dama_ctrl->buildTTP(ttp);
}

void SpotDownward::updateStatistics(void)
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
			this->l2_to_sat_bytes_before_sched[(*it).first] * 8.0 /
			this->stats_period_ms);
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

	this->resetStatsCxt();
}

void SpotDownward::resetStatsCxt(void)
{
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		this->l2_to_sat_bytes_before_sched[(*it).first] = 0;
	}
	this->l2_to_sat_total_bytes = 0;
}


double SpotDownward::getCni(void) const
{
	return this->cni;
}

void SpotDownward::setCni(double cni)
{
	this->cni = cni;
}

bool SpotDownward::handleSac(const DvbFrame *dvb_frame)
{
	Sac *sac = (Sac *)dvb_frame;

	LOG(this->log_receive_channel, LEVEL_DEBUG,
			"handle received SAC\n");

	if(!this->dama_ctrl->hereIsSAC(sac))
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
				"failed to handle SAC frame\n");
		delete dvb_frame;
		return false;
	}

	if(this->with_phy_layer)
	{
		// transparent : the C/N0 of forward link
		// regenerative : the C/N0 of uplink (updated by sat)
		double cni = sac->getCni();
		tal_id_t tal_id = sac->getTerminalId();
		if(this->satellite_type == TRANSPARENT)
		{
			this->down_fwd_fmt_simu.setRequiredModcod(tal_id,
					cni);
		}
		else
		{
			this->up_ret_fmt_simu.setRequiredModcod(tal_id,
					cni);
		}
	}
	return true;
}

bool SpotDownward::checkDama()
{
	if(!this->dama_ctrl)
	{
		// stop here
		return true;
	}
	return false;
}


bool SpotDownward::handleFrameTimer(time_sf_t super_frame_counter)
{
	// Upate the superframe counter
	this->super_frame_counter = super_frame_counter;

	

	if(this->with_phy_layer)
	{
		// for each terminal in DamaCtrl update FMT because in
		// this case this it not done with scenario timer and
		// FMT is updated each received frame but we only need
		// it for allocation
		this->dama_ctrl->updateFmt();
	}

	// run the allocation algorithms (DAMA)
	this->dama_ctrl->runOnSuperFrameChange(this->super_frame_counter);

	// **************
	// Simulation
	// **************
	switch(this->simulate)
	{
		case file_simu:
			if(!this->simulateFile())
			{
				LOG(this->log_request_simulation,
				    LEVEL_ERROR, "file simulation failed");
				fclose(this->simu_file);
				this->simu_file = NULL;
				this->simulate = none_simu;
				return false;
			}
			break;
		case random_simu:
			if(!this->simu_eof && !this->simulateRandom())
			{
				this->simulate = none_simu;
				return false;
			}
			break;
		default:
			break;
	}
	// flush files
	fflush(this->event_file);
	return true;
}

bool SpotDownward::applyPepCommand(PepRequest *pep_request)
{
	if(this->dama_ctrl->applyPepCommand(pep_request))
	{
		LOG(this->log_receive_channel, LEVEL_NOTICE,
		    "PEP request successfully "
		    "applied in DAMA\n");
	}
	else
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "failed to apply PEP request "
		    "in DAMA\n");
		return false;
	}

	return true;
}

void SpotDownward::updateFmt(void)
{
	if(!this->dama_ctrl)
	{
		// stop here
		return;
	}
	// TODO FMT in slotted aloha should be handled on ST
	//  => so remove return fmt simu !
	//  => keep this todo in order to think of it on ST

	// for each terminal in DamaCtrl update FMT
	this->dama_ctrl->updateFmt();
}

uint8_t SpotDownward::getCtrlCarrierId(void) const
{
	return this->ctrl_carrier_id;
}

uint8_t SpotDownward::getSofCarrierId(void) const
{
	return this->sof_carrier_id;
}

uint8_t SpotDownward::getDataCarrierId(void) const
{
	return this->data_carrier_id;
}

list<DvbFrame *> &SpotDownward::getCompleteDvbFrames(void)
{
	return this->complete_dvb_frames;
}

event_id_t SpotDownward::getPepCmdApplyTimer(void)
{
	return this->pep_cmd_apply_timer;
}

void SpotDownward::setPepCmdApplyTimer(event_id_t pep_cmd_a_timer)
{
	this->pep_cmd_apply_timer = pep_cmd_a_timer;
}
