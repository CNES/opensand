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
 * @file SpotUpward.cpp
 * @brief This bloc implements a DVB-S/RCS stack for a Ncc.
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 */


#include "SpotUpward.h"

#include "DamaCtrlRcsLegacy.h"

#include "DvbRcsStd.h"
#include "DvbS2Std.h"
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
 *        - upward => return link
 *  // in regenerative mode
 *        - upward => downlink
 */

/*****************************************************************************/
/*                               SpotUpward                                      */
/*****************************************************************************/

SpotUpward::SpotUpward():
	saloha(NULL),
	mac_id(GW_TAL_ID),
	ret_fmt_groups(),
	probe_gw_l2_from_sat(NULL),
	probe_received_modcod(NULL),
	probe_rejected_modcod(NULL),
	log_saloha(NULL),
	event_logon_req(NULL)
{
}


SpotUpward::~SpotUpward()
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


bool SpotUpward::onInit(void)
{
	string scheme;

	if(!this->initSatType())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
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
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		goto error;
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

	LOG(this->log_init_channel, LEVEL_DEBUG,
	    "SF#%u Link is up msg sent to upper layer\n",
	    this->super_frame_counter);

	// everything went fine
	return true;

error_mode:
	delete this->reception_std;
error:
	return false;
}

bool SpotUpward::initSlottedAloha(void)
{
	TerminalCategories<TerminalCategorySaloha> sa_categories;
	TerminalMapping<TerminalCategorySaloha> sa_terminal_affectation;
	TerminalCategorySaloha *sa_default_category;
	int lan_scheme_nbr;

	// init fmt_simu
	if(!this->initModcodFiles(RETURN_UP_MODCOD_DEF_RCS,
	                          RETURN_UP_MODCOD_TIME_SERIES))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the up/return MODCOD files\n");
		return false;
	}
	
	ConfigurationList return_up_band = Conf::section_map[RETURN_UP_BAND];
	ConfigurationList spots;
	ConfigurationList current_spot;
	if(!Conf::getListNode(return_up_band, SPOT_LIST, spots))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no %s into %s section", 
		    SPOT_LIST, RETURN_UP_BAND);
		return false;
	}

	char s_id[10]; 
	sprintf (s_id, "%d", this->spot_id);
	if(!Conf::getElementWithAttributeValue(spots, SPOT_ID, 
	                                      s_id, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no attribute %s with value: %d into %s/%s", 
		    SPOT_ID, this->spot_id, RETURN_UP_BAND, SPOT_LIST);
		return false;
	}
	
	if(!this->initBand<TerminalCategorySaloha>(current_spot,
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
		LOG(this->log_init_channel, LEVEL_DEBUG,
		    "No Slotted Aloha carrier\n");
		return true;
	}

	// cannot use Slotted Aloha with regenerative satellite
	if(this->satellite_type == REGENERATIVE)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Carrier configured with Slotted Aloha while satellite "
		    "is regenerative\n");
		return false;
	}

	// TODO possible loss with Slotted Aloha and ROHC or MPEG
	//      (see TODO in TerminalContextSaloha.cpp)
	if(this->pkt_hdl->getName() == "MPEG2-TS")
	{
		LOG(this->log_init_channel, LEVEL_WARNING,
		    "Cannot guarantee no loss with MPEG2-TS and Slotted Aloha "
		    "on return link due to interleaving\n");
	}
	if(!Conf::getNbListItems(Conf::section_map[GLOBAL_SECTION],
		                     LAN_ADAPTATION_SCHEME_LIST,
	                         lan_scheme_nbr))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Section %s, %s missing\n", GLOBAL_SECTION,
		    LAN_ADAPTATION_SCHEME_LIST);
		return false;
	}
	for(int i = 0; i < lan_scheme_nbr; i++)
	{
		string name;
		if(!Conf::getValueInList(Conf::section_map[GLOBAL_SECTION],
			                     LAN_ADAPTATION_SCHEME_LIST,
		                         POSITION, toString(i), PROTO, name))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "Section %s, invalid value %d for parameter '%s'\n",
			    GLOBAL_SECTION, i, POSITION);
			return false;
		}
		if(name == "ROHC")
		{
			LOG(this->log_init_channel, LEVEL_WARNING,
			    "Cannot guarantee no loss with RoHC and Slotted Aloha "
			    "on return link due to interleaving\n");
		}
	}
	// end TODO

	// Create the Slotted ALoha part
	this->saloha = new SlottedAlohaNcc();
	if(!this->saloha)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
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
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Dama Controller Initialization failed.\n");
		goto release_saloha;
	}

	if(!this->saloha->init(sa_categories,
	                       sa_terminal_affectation,
	                       sa_default_category,
	                       this->spot_id))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the DAMA controller\n");
		goto release_saloha;
	}

	return true;

release_saloha:
	delete this->saloha;
	return false;
}


bool SpotUpward::initMode(void)
{
	// initialize the reception standard
	// depending on the satellite type
	if(this->satellite_type == TRANSPARENT)
	{
		this->reception_std = new DvbRcsStd(this->pkt_hdl);
	}
	else if(this->satellite_type == REGENERATIVE)
	{
		this->reception_std = new DvbS2Std(this->pkt_hdl);
	}
	else
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "unknown value '%u' for satellite type ",
		    this->satellite_type);
		goto error;

	}
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


bool SpotUpward::initOutput(void)
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

	if(this->satellite_type == REGENERATIVE)
	{
		this->probe_received_modcod = Output::registerProbe<int>("modcod index",
		                                                         true, SAMPLE_LAST,
		                                                         "Spot_%d.ACM.Received_modcod",
		                                                         this->spot_id);
		this->probe_rejected_modcod = Output::registerProbe<int>("modcod index",
		                                                         true, SAMPLE_LAST,
		                                                         "Spot_%d.ACM.Rejected_modcod",
		                                                         this->spot_id);
	}
	return true;
}


bool SpotUpward::onRcvLogonReq(DvbFrame *dvb_frame)
{
	//TODO find why dynamic cast fail here !?
//	LogonRequest *logon_req = dynamic_cast<LogonRequest *>(dvb_frame);
	LogonRequest *logon_req = (LogonRequest *)dvb_frame;
	uint16_t mac = logon_req->getMac();

	LOG(this->log_receive_channel, LEVEL_INFO,
	    "Logon request from ST%u\n", mac);

	// refuse to register a ST with same MAC ID as the NCC
	if(mac == this->mac_id)
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
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
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "Cannot add terminal in Slotted Aloha context\n");
			return false;
		}
	}

	// send the corresponding event
	Output::sendEvent(this->event_logon_req,
	                  "Logon request received from %u",
	                  mac);

	return true;
}

void SpotUpward::updateStats(void)
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

void SpotUpward::setSpotId(uint8_t spot_id)
{
	this->spot_id = spot_id;
}

uint8_t SpotUpward::getSpotId(void)
{
	return this->spot_id;
}

PhysicStd *SpotUpward::getReceptionStd(void)
{
	return this->reception_std;
}

SlottedAlohaNcc *SpotUpward::getSaloha(void)
{
	return this->saloha;
}

Probe<int> *SpotUpward::getProbeReceivedModcod(void)
{
	return this->probe_received_modcod;
}

Probe<int> *SpotUpward::getProbeRejectedModcod(void)
{
	return this->probe_rejected_modcod;
}

void SpotUpward::setL2FromSatBytes(int l2_from_sat)
{
	this->l2_from_sat_bytes = l2_from_sat;
}

int SpotUpward::getL2FromSatBytes(void)
{
	return this->l2_from_sat_bytes;
}

tal_id_t SpotUpward::getMacId(void)
{
	return this->mac_id;
}
