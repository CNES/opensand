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
 * @file SpotUpwardRegen.cpp
 * @brief Upward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#include "SpotUpwardRegen.h"

#include "DamaCtrlRcsLegacy.h"

#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "Sof.h"

SpotUpwardRegen::SpotUpwardRegen(spot_id_t spot_id,
                                 tal_id_t mac_id,
                                 StFmtSimuList *input_sts,
                                 StFmtSimuList *output_sts):
	SpotUpward(spot_id, mac_id, input_sts, output_sts)
{
}


SpotUpwardRegen::~SpotUpwardRegen()
{
}


bool SpotUpwardRegen::onInit(void)
{
	string scheme = FORWARD_DOWN_ENCAP_SCHEME_LIST;

	// get the common parameters
	if(!this->initCommon(scheme.c_str()))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		return false;
	}

	if(!SpotUpward::onInit())
	{
		return false;
	}

	return true;

}

bool SpotUpwardRegen::initModcodSimu(void)
{
	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            this->input_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the forward MODCOD file\n");
		return false;
	}
	if(!this->initModcodDefFile(MODCOD_DEF_RCS,
	                            this->output_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the uplink MODCOD file\n");
		return false;
	}

	if(!this->initModcodSimuFile(FORWARD_DOWN_MODCOD_TIME_SERIES,
	                             this->mac_id, this->spot_id))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the downlink MODCOD files\n");
		return false;
	}

	// initialize the MODCOD IDs
	if(!this->fmt_simu.goFirstScenarioStep())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize MODCOD scheme IDs\n");
		return false;
	}

	// declare the GW as one ST for the MODCOD scenarios
	if(!this->addInputTerminal(this->mac_id, this->mac_id, this->spot_id))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to define the GW as ST with ID %d\n",
		    this->mac_id);
		return false;
	}
	if(!this->addOutputTerminal(this->mac_id, this->mac_id, this->spot_id))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to define the GW as ST with ID %d\n",
		    this->mac_id);
		return false;
	}

	return true;
}

bool SpotUpwardRegen::initSeriesGenerator(void)
{
	string generate;
	ConfigurationList current_gw;
	string input_file;
	string output_file;
	vector<string> path_split;

	if(!this->with_phy_layer)
	{
		return true;
	}

	// Check whether we generate the time series
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION],
	                   GENERATE_TIME_SERIES_PATH, generate))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    PHYSICAL_LAYER_SECTION, GENERATE_TIME_SERIES_PATH);
		return false;
	}
	if(generate == "none")
	{
		return true;
	}

	// load the time series filenames
	if(!OpenSandConf::getSpot(PHYSICAL_LAYER_SECTION,
	                          this->spot_id, this->mac_id, current_gw))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s', missing spot for id %d and gw %d\n",
		    PHYSICAL_LAYER_SECTION, this->spot_id, this->mac_id);
		return false;
	}

	if(!Conf::getValue(current_gw, RETURN_UP_MODCOD_TIME_SERIES,
	                   input_file))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s/spot_%d_gw_%d', missing section '%s'\n",
		    PHYSICAL_LAYER_SECTION, this->spot_id, this->mac_id,
		    RETURN_UP_MODCOD_TIME_SERIES);
		return false;
	}

	// extract the filename from path
	tokenize(input_file, path_split, "/");
	input_file = generate + "/" + path_split.back();

	if(!Conf::getValue(current_gw, FORWARD_DOWN_MODCOD_TIME_SERIES,
	                   output_file))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s/spot_%d_gw_%d', missing section '%s'\n",
		    PHYSICAL_LAYER_SECTION, this->spot_id, this->mac_id,
		    FORWARD_DOWN_MODCOD_TIME_SERIES);
		return false;
	}

	// extract the filename from path
	tokenize(output_file, path_split, "/");
	output_file = generate + "/" + path_split.back();


	this->input_series = new TimeSeriesGenerator(input_file);
	this->output_series = new TimeSeriesGenerator(output_file);
	return true;
}


bool SpotUpwardRegen::initMode(void)
{
	this->reception_std = new DvbS2Std(this->pkt_hdl);
	
	if(!this->reception_std)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to create the reception standard\n");
		goto error;
	}

	return true;

error:
	return false;
}


bool SpotUpwardRegen::initOutput(void)
{
	// Events
	this->event_logon_req = Output::registerEvent("Spot_%d.DVB.logon_request",
	                                                 this->spot_id);

	// Output probes and stats
	this->probe_gw_l2_from_sat=
		Output::registerProbe<int>("Kbits/s", true, SAMPLE_AVG,
		                           "Spot_%d.Throughputs.L2_from_SAT",
		                            this->spot_id);
	this->l2_from_sat_bytes = 0;

	this->probe_received_modcod = Output::registerProbe<int>("modcod index",
	                                                         true, SAMPLE_LAST,
	                                                         "Spot_%d.ACM.Received_modcod",
	                                                         this->spot_id);
	this->probe_rejected_modcod = Output::registerProbe<int>("modcod index",
	                                                         true, SAMPLE_LAST,
	                                                         "Spot_%d.ACM.Rejected_modcod",
	                                                         this->spot_id);
	return true;
}

bool SpotUpwardRegen::handleFrame(DvbFrame *frame, NetBurst **burst)
{
	uint8_t msg_type = frame->getMessageType();
	PhysicStd *std = this->reception_std;
	
	// Update stats
	this->l2_from_sat_bytes += frame->getPayloadLength();

	if(!std->onRcvFrame(frame, this->mac_id, burst))
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "failed to handle DVB frame or BB frame\n");
		return false;
	}

	if(std->getType() == "DVB-S2")
	{
		DvbS2Std *s2_std = (DvbS2Std *)std;
		if(msg_type != MSG_TYPE_CORRUPTED)
		{
			this->probe_received_modcod->put(s2_std->getReceivedModcod());
		}
		else
		{
			this->probe_rejected_modcod->put(s2_std->getReceivedModcod());
		}
	}

	return true;
}

