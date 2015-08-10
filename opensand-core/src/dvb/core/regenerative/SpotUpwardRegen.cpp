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

	if(!this->initSatType())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize satellite type\n");
		goto error;
	}

	if(!this->initCommon(scheme.c_str()))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		goto error;
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

	// initialize the slotted Aloha part
	if(!this->initSlottedAloha())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the DAMA part of the "
		    "initialisation\n");
		goto error;
	}

	// synchronized with SoF
	this->initStatsTimer(this->ret_up_frame_duration_ms);

	if(!this->initOutput())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the initialization of "
		    "statistics\n");
		goto error_mode;
	}

	// everything went fine
	return true;

error_mode:
	delete this->reception_std;
error:
	return false;
}

bool SpotUpwardRegen::initSlottedAloha(void)
{
	TerminalCategories<TerminalCategorySaloha> sa_categories;
	TerminalMapping<TerminalCategorySaloha> sa_terminal_affectation;
	TerminalCategorySaloha *sa_default_category;
	ConfigurationList current_spot;

	if(!OpenSandConf::getSpot(RETURN_UP_BAND, this->spot_id, 
		                      NO_GW, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no attribute %s with value %d and %s with value %d into %s/%s\n",
		    ID, this->spot_id, GW, this->mac_id, RETURN_UP_BAND, SPOT_LIST);
		return false;
	}
	
	if(!this->initBand<TerminalCategorySaloha>(current_spot,
	                                           RETURN_UP_BAND,
	                                           ALOHA,
	                                           this->ret_up_frame_duration_ms,
	                                           this->satellite_type,
	                                           &this->output_modcod_def,
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
		LOG(this->log_init_channel, LEVEL_DEBUG,
		    "No Slotted Aloha carrier\n");
		return true;
	}
	
	LOG(this->log_init_channel, LEVEL_ERROR,
	    "Carrier configured with Slotted Aloha while satellite "
	    "is regenerative\n");
	return false;

}


bool SpotUpwardRegen::initModcodSimu(void)
{
	if(!this->initModcodDefFile(FORWARD_DOWN_MODCOD_DEF_S2,
	                            this->input_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the forward MODCOD file\n");
		return false;
	}
	if(!this->initModcodDefFile(RETURN_UP_MODCOD_DEF_RCS,
	                            this->output_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the uplink MODCOD file\n");
		return false;
	}

	if(!this->initModcodFiles(FORWARD_DOWN_MODCOD_TIME_SERIES,
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

	if(this->saloha)
	{
		this->log_saloha = Output::registerLog(LEVEL_WARNING, "Spot_%d.Dvb.SlottedAloha",
		                                       this->spot_id);
	}

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

	// TODO MODCOD should also be updated correctly for SCPC but at the moment
	//      FMT simulations cannot handle this, fix this once this
	//      will be reworked
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

bool SpotUpwardRegen::scheduleSaloha(DvbFrame *dvb_frame,
                                list<DvbFrame *>* &ack_frames,
                                NetBurst **sa_burst)
{
	if(!this->saloha)
	{
		return true;
	}
	uint16_t sfn;
	Sof *sof = (Sof *)dvb_frame;

	sfn = sof->getSuperFrameNumber();

	ack_frames = new list<DvbFrame *>();
	// increase the superframe number and reset
	// counter of frames per superframe
	this->super_frame_counter++;
	if(this->super_frame_counter != sfn)
	{
		LOG(this->log_receive_channel, LEVEL_WARNING,
			"superframe counter (%u) is not the same as in"
			" SoF (%u)\n",
			this->super_frame_counter, sfn);
		this->super_frame_counter = sfn;
	}

	if(!this->saloha->schedule(sa_burst,
	                           *ack_frames,
	                           this->super_frame_counter))
	{
		LOG(this->log_saloha, LEVEL_ERROR,
		    "failed to schedule Slotted Aloha\n");
		delete dvb_frame;
		delete ack_frames;
		return false;
	}

	return true;
}

bool SpotUpwardRegen::handleSlottedAlohaFrame(DvbFrame *frame)
{
	// Update stats
	this->l2_from_sat_bytes += frame->getPayloadLength();

	if(!this->saloha->onRcvFrame(frame))
	{
		LOG(this->log_saloha, LEVEL_ERROR,
		    "failed to handle Slotted Aloha frame\n");
		return false;
	}
	return true;
}


