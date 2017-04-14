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

	if(!this->initModcodDefinitionTypes())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize MOCODS definitions types\n");
		return false;
	}
	
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
	if(!this->initModcodDefFile(this->modcod_def_rcs_type.c_str(),
	                            &this->rcs_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the uplink definition MODCOD file\n");
		return false;
	}
	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            &this->s2_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the downlink definition MODCOD file\n");
		return false;
	}

	if(!this->initModcodSimuFile(FORWARD_DOWN_MODCOD_TIME_SERIES,
	                             this->mac_id, this->spot_id))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the downlink simulation MODCOD files\n");
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
	if(!this->addInputTerminal(this->mac_id, this->s2_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to define the GW as ST with ID %d\n",
		    this->mac_id);
		return false;
	}
	if(!this->addOutputTerminal(this->mac_id, this->rcs_modcod_def))
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


bool SpotUpwardRegen::initAcmLoopMargin(void)
{
	double up_acm_margin_db;
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION],
	                   RETURN_UP_ACM_LOOP_MARGIN,
	                   up_acm_margin_db))
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    PHYSICAL_LAYER_SECTION, RETURN_UP_ACM_LOOP_MARGIN);
		return false;
	}
	this->output_sts->setAcmLoopMargin(up_acm_margin_db);

	return true;
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

bool SpotUpwardRegen::onRcvLogonReq(DvbFrame *dvb_frame)
{
	if(!SpotUpward::onRcvLogonReq(dvb_frame))
	{
		return false;
	}

	LogonRequest *logon_req = (LogonRequest *)dvb_frame;
	uint16_t mac = logon_req->getMac();

	// handle ST for FMT simulation
	if(!(this->input_sts->isStPresent(mac) && this->output_sts->isStPresent(mac)))
	{
		// ST was not registered yet
		if(!this->addInputTerminal(mac, this->s2_modcod_def))
		{
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "failed to handle FMT for ST %u, "
			    "won't send logon response\n", mac);
			return false;
		}
		if(!this->addOutputTerminal(mac, this->rcs_modcod_def))
		{
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "failed to handle FMT for ST %u, "
			    "won't send logon response\n", mac);
			return false;
		}
	}

	return true;
}



bool SpotUpwardRegen::handleFrame(DvbFrame *frame, NetBurst **burst)
{
	bool corrupted = frame->isCorrupted();
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
		if(!corrupted)
		{
			this->probe_received_modcod->put(s2_std->getReceivedModcod());
			this->probe_rejected_modcod->put(0);
		}
		else
		{
			this->probe_rejected_modcod->put(s2_std->getReceivedModcod());
			this->probe_received_modcod->put(0);
		}
	}

	return true;
}

void SpotUpwardRegen::handleFrameCni(DvbFrame *dvb_frame)
{
	if(!this->with_phy_layer)
	{
		return;
	}

	double cni = dvb_frame->getCn();
	// regenerative case:
	//   we need downlink ACM parameters to inform
	//   satellite with a SAC
	this->setRequiredCniInput(this->mac_id, cni);
}


bool SpotUpwardRegen::updateSeriesGenerator(void)
{
	return true;
}
