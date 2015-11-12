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
 * @file SpotDownwardTransp.cpp
 * @brief Downward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#include "SpotDownwardTransp.h"

#include "ForwardSchedulingS2.h"
#include "DamaCtrlRcsLegacy.h"

#include <errno.h>


SpotDownwardTransp::SpotDownwardTransp(spot_id_t spot_id,
                           tal_id_t mac_id,
                           time_ms_t fwd_down_frame_duration,
                           time_ms_t ret_up_frame_duration,
                           time_ms_t stats_period,
                           sat_type_t sat_type,
                           EncapPlugin::EncapPacketHandler *pkt_hdl,
                           StFmtSimuList *input_sts,
                           StFmtSimuList *output_sts):
	SpotDownward(spot_id, mac_id, 
	             fwd_down_frame_duration, 
	             ret_up_frame_duration, 
	             stats_period, sat_type,
	             pkt_hdl, input_sts, output_sts)
{
}

SpotDownwardTransp::~SpotDownwardTransp()
{
	this->categories.clear();
}


bool SpotDownwardTransp::onInit(void)
{
	if(!this->initPktHdl(RETURN_UP_ENCAP_SCHEME_LIST,
	                     &this->up_return_pkt_hdl, false))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed get packet handler\n");
		return false;
	}

	// Initialization of the modcod def
	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            &this->output_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the forward MODCOD file\n");
		return false;
	}
	if(!this->initModcodDefFile(MODCOD_DEF_RCS,
	                            &this->input_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the forward MODCOD file\n");
		return false;
	}
	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            &this->input_modcod_def_scpc))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the forward MODCOD file\n");
		return false;
	}
	
	if(!SpotDownward::onInit())
	{
		return false;
	}
	return true;

}


bool SpotDownwardTransp::initMode(void)
{
	TerminalCategoryDama *cat;

	// initialize scheduling
	// depending on the satellite type
	ConfigurationList forward_down_band = Conf::section_map[FORWARD_DOWN_BAND];
	ConfigurationList spots;
	ConfigurationList current_spot;
	ConfigurationList current_gw;
	const ListStFmt *list;

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
	                                         this->output_modcod_def,
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

	list = this->output_sts->getListSts();
	cat = this->categories.begin()->second;
	this->scheduling = new ForwardSchedulingS2(this->fwd_down_frame_duration_ms,
	                                           this->pkt_hdl,
	                                           this->dvb_fifos,
	                                           list,
	                                           this->output_modcod_def,
	                                           cat, this->spot_id,
	                                           true, this->mac_id, "");


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


// TODO this function is NCC part but other functions are related to GW,
//      we could maybe create two classes inside the block to keep them separated
bool SpotDownwardTransp::initDama(void)
{
	time_ms_t sync_period_ms;
	time_frame_t sync_period_frame;
	time_sf_t rbdc_timeout_sf;
	rate_kbps_t fca_kbps;
	string dama_algo;
	const ListStFmt *list;

	TerminalCategories<TerminalCategoryDama> dc_categories;
	TerminalMapping<TerminalCategoryDama> dc_terminal_affectation;
	TerminalCategoryDama *dc_default_category;

	ConfigurationList current_gw;
	
	// Retrieving the free capacity assignement parameter
	if(!Conf::getValue(Conf::section_map[DC_SECTION_NCC],
		               DC_FREE_CAP, fca_kbps))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "missing %s parameter\n", DC_FREE_CAP);
		return false;
	}
	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "fca = %d kb/s\n", fca_kbps);

	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
		               SYNC_PERIOD, sync_period_ms))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Missing %s\n", SYNC_PERIOD);
		return false;
	}
	sync_period_frame = (time_frame_t)round((double)sync_period_ms /
	                                        (double)this->ret_up_frame_duration_ms);
	rbdc_timeout_sf = sync_period_frame + 1;

	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "rbdc_timeout = %d superframes computed from sync period %d superframes\n",
	    rbdc_timeout_sf, sync_period_frame);

	if(!OpenSandConf::getSpot(RETURN_UP_BAND,
		                      this->spot_id, 
		                      this->mac_id, current_gw))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s', missing spot for id %d and gw is %d\n",
		    RETURN_UP_BAND, this->spot_id, this->mac_id);
		return false;
	}

	if(!this->initBand<TerminalCategoryDama>(current_gw,
	                                         RETURN_UP_BAND,
	                                         DAMA,
	                                         this->ret_up_frame_duration_ms,
	                                         this->satellite_type,
	                                         this->input_modcod_def,
	                                         dc_categories,
	                                         dc_terminal_affectation,
	                                         &dc_default_category,
	                                         this->ret_fmt_groups))
	{
		return false;
	}
	

	// check if there is DAMA carriers
	if(dc_categories.size() == 0)
	{
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
		return false;
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
		return false;
	}

	if(!this->dama_ctrl)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to create the DAMA controller\n");
		return false;
	}
	
	if(!this->up_return_pkt_hdl)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "up return pkt hdl has not been initialized first.\n");
		return false;
	}

	// Initialize the DamaCtrl parent class
	list = this->input_sts->getListSts();
	if(!this->dama_ctrl->initParent(this->ret_up_frame_duration_ms,
	                                this->with_phy_layer,
	                                this->up_return_pkt_hdl->getFixedLength(),
	                                rbdc_timeout_sf,
	                                fca_kbps,
	                                dc_categories,
	                                dc_terminal_affectation,
	                                dc_default_category,
	                                list,
	                                this->input_modcod_def,
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
	return false;
}


bool SpotDownwardTransp::handleSalohaAcks(const list<DvbFrame *> *ack_frames)
{
	list<DvbFrame *>::const_iterator ack_it;
	for(ack_it = ack_frames->begin(); ack_it != ack_frames->end();
	    ++ack_it)
	{
		this->complete_dvb_frames.push_back(*ack_it);
	}
	return true;
}
