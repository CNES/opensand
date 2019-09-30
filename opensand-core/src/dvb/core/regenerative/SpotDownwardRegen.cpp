/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @file SpotDownwardRegen.cpp
 * @brief Downward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmurguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 *
 */

#include "SpotDownwardRegen.h"

#include "UplinkSchedulingRcsCommon.h"
#include "UplinkSchedulingRcs.h"
#include "UplinkSchedulingRcs2.h"
#include "DamaCtrlRcsLegacy.h"
#include "DamaCtrlRcs2Legacy.h"

#include <errno.h>


SpotDownwardRegen::SpotDownwardRegen(spot_id_t spot_id,
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

SpotDownwardRegen::~SpotDownwardRegen()
{
}


bool SpotDownwardRegen::onInit(void)
{
	this->up_return_pkt_hdl = this->pkt_hdl;

	if(!this->initModcodDefinitionTypes())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize MOCODS definitions types\n");
		return false;
	}

	// Initialization of the modcod def
	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            &this->s2_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the uplink definition MODCOD file\n");
		return false;
	}
	// we use RCS as input because we will consider
	// the terminal to satellite link and not the satellite
	// to GW link
	if(!this->initModcodDefFile(this->modcod_def_rcs_type.c_str(),
	                            &this->rcs_modcod_def,
	                            this->req_burst_length))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the downlink definition MODCOD file\n");
		return false;
	}

	if(!SpotDownward::onInit())
	{
		return false;
	}
	return true;
}


bool SpotDownwardRegen::initMode(void)
{
	TerminalCategories<TerminalCategoryDama>::iterator cat_it;

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

	// get the spot which have the same id as SpotDownwardRegen
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
	                                         this->rcs_modcod_def,
	                                         this->categories,
	                                         this->terminal_affectation,
	                                         &this->default_category,
	                                         this->ret_fmt_groups))
	{
		return false;
	}

	for(cat_it = this->categories.begin();
		cat_it != this->categories.end(); ++cat_it)
	{
		fifos_t fifos;
		string label;
		UplinkSchedulingRcsCommon *schedule = NULL;
		TerminalCategoryDama *cat;

		cat = cat_it->second;
		label = cat->getLabel();
		if(!this->initFifo(fifos))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
				"failed to initialize fifos for category %s\n", label.c_str());
			return false;
		}
		this->dvb_fifos.insert(
			make_pair<string, fifos_t>((string)label, (fifos_t)fifos));

		if(this->return_link_std == DVB_RCS)
		{
			schedule = new UplinkSchedulingRcs(this->ret_up_frame_duration_ms,
				this->pkt_hdl,
				this->dvb_fifos.at(label),
				this->output_sts,
				this->rcs_modcod_def,
				cat,
				this->mac_id);
		}
		else if(this->return_link_std == DVB_RCS2)
		{
			schedule = new UplinkSchedulingRcs2(this->ret_up_frame_duration_ms,
				this->pkt_hdl,
				this->dvb_fifos.at(label),
				this->output_sts,
				this->rcs_modcod_def,
				cat,
				this->mac_id);
		}
		else
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
				"Unable to create the uplink scheduling for standard '%s'\n",
				this->return_link_std_str.c_str());
			return false;
		}
		if(!schedule || !schedule->init())
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
				"failed to complete the SCHEDULE part of the "
				"initialisation\n");
			return false;
		}

		this->scheduling.insert(
			make_pair<string, Scheduling *>((string)label, (Scheduling *)schedule));
	}

	return true;
}


// TODO this function is NCC part but other functions are related to GW,
//      we could maybe create two classes inside the block to keep them separated
bool SpotDownwardRegen::initDama(void)
{
	time_ms_t sync_period_ms;
	time_frame_t sync_period_frame;
	time_sf_t rbdc_timeout_sf;
	rate_kbps_t fca_kbps;
	string dama_algo;

	TerminalCategories<TerminalCategoryDama> dc_categories;
	TerminalMapping<TerminalCategoryDama> dc_terminal_affectation;
	TerminalCategoryDama *dc_default_category;

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


	// band already initialized in initMode
	dc_categories = this->categories;
	dc_terminal_affectation = this->terminal_affectation;
	dc_default_category = this->default_category;

	// check if there is DAMA carriers
	if(dc_categories.size() == 0)
	{
		// No Slotted Aloha with regenerative satellite,
		// so we need a DAMA
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "No DAMA and regenerative satellite\n");
		return false;
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
		if(this->return_link_std == DVB_RCS)
		{
			this->dama_ctrl = new DamaCtrlRcsLegacy(this->spot_id,
			                                        this->up_return_pkt_hdl->getFixedLength() << 3);
		}
		else if(this->return_link_std == DVB_RCS2)
		{
			this->dama_ctrl = new DamaCtrlRcs2Legacy(this->spot_id);
		}
		else
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
				"section '%s': bad value '%s' for parameter '%s'"
				" (no matching dama controller for return link standard '%s')\n",
				DVB_NCC_SECTION, dama_algo.c_str(), DVB_NCC_DAMA_ALGO,
				this->return_link_std_str.c_str());
			return false;
		}
	}
	else
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s': bad value '%s' for parameter '%s'\n",
			DVB_NCC_SECTION, dama_algo.c_str(), DVB_NCC_DAMA_ALGO);
		return false;
	}

	if(!this->dama_ctrl)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to create the DAMA controller\n");
		return false;
	}

	// Initialize the DamaCtrl parent class
	// Here we use output STs because GW has the same
	// output link standard than terminals and required modcod for
	// terminals is received in SAC and added to output STs
	if(!this->dama_ctrl->initParent(this->ret_up_frame_duration_ms,
	                                rbdc_timeout_sf,
	                                fca_kbps,
	                                dc_categories,
	                                dc_terminal_affectation,
	                                dc_default_category,
	                                this->output_sts,
	                                this->rcs_modcod_def,
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



bool SpotDownwardRegen::initOutput(void)
{
	if(!SpotDownward::initOutput())
	{
		return false;
	}

	char probe_name[128];
	snprintf(probe_name, sizeof(probe_name), "Spot_%d.Down_Forward_modcod.Sent_modcod", this->spot_id);
	this->probe_sent_modcod = Output::Get()->registerProbe<int>(probe_name, "modcod index", true, SAMPLE_LAST);

	return true;
}

bool SpotDownwardRegen::handleFwdFrameTimer(time_sf_t fwd_frame_counter)
{
	if(!SpotDownward::handleFwdFrameTimer(fwd_frame_counter))
	{
		return false;
	}

	if(this->complete_dvb_frames.size() > 0)
	{
		// we can do that because we have only one MODCOD per allocation
		// TODO THIS IS NOT TRUE ! we schedule for each carriers, if
		// desired modcod is low we can send on many carriers
		uint8_t modcod_id;
		modcod_id = ((DvbRcsFrame *)this->complete_dvb_frames.front())->getModcodId();
		this->probe_sent_modcod->put(modcod_id);
	}

	return true;
}


bool SpotDownwardRegen::addCniExt(void)
{
	return true;
}

