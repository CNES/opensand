/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 CNES
 * Copyright © 2019 TAS
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
 * @file AttenuationHandler.cpp
 * @brief Process the attenuation
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 * @author Aurélien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include "AttenuationHandler.h"
#include "Plugin.h"
#include "BBFrame.h"
#include "DvbRcsFrame.h"
#include "OpenSandCore.h"

#include <opensand_output/Output.h>
#include <opensand_conf/conf.h>

#include <math.h>

AttenuationHandler::AttenuationHandler(OutputLog *log_channel):
	minimal_condition_model(NULL),
	error_insertion_model(NULL),
	log_channel(log_channel),
	probe_minimal_condition(NULL),
	probe_drops(NULL)
{
}

AttenuationHandler::~AttenuationHandler()
{
}

bool AttenuationHandler::initialize(const string &link_section, OutputLog *log_init)
{
	string minimal_type;
	string error_type;

	// Get parameters
	if(!Conf::getValue(Conf::section_map[link_section],
	                   MINIMAL_CONDITION_TYPE,
	                   minimal_type))
	{
		LOG(log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'",
		    link_section.c_str(),
		    MINIMAL_CONDITION_TYPE);
		return false;
	}
	LOG(log_init, LEVEL_NOTICE,
	    "minimal_condition_type = %s", minimal_type.c_str());
	if(!Conf::getValue(Conf::section_map[link_section],
	                   ERROR_INSERTION_TYPE,
	                   error_type))
	{
		LOG(log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'",
		    link_section.c_str(),
		    ERROR_INSERTION_TYPE);
		return false;
	}
	LOG(log_init, LEVEL_NOTICE,
	    "error_insertion_type = %s", error_type.c_str());

	// Load plugins
	if(!Plugin::getMinimalConditionPlugin(minimal_type, &this->minimal_condition_model))
	{
		LOG(log_init, LEVEL_ERROR,
		    "Unable to get the physical layer minimal condition plugin");
		return false;
	}
	if(!Plugin::getErrorInsertionPlugin(error_type, &this->error_insertion_model))
	{
		LOG(log_init, LEVEL_ERROR,
		    "Unable to get the physical layer error insertion plugin");
		return false;
	}

	// Initialize plugins
	if(!this->minimal_condition_model->init())
	{
		LOG(log_init, LEVEL_ERROR,
		    "Unable to initialize the physical layer minimal condition plugin %s",
		    minimal_type.c_str());
		return false;
	}
	if(!this->error_insertion_model->init())
	{
		LOG(log_init, LEVEL_ERROR,
		    "Unable to initialize the physical layer error insertion plugin %s",
		    minimal_type.c_str());
		return false;
	}

	// Initialize probes
	this->probe_minimal_condition = Output::registerProbe<float>("Phy.minimal_condition",
	                                                             "dB", true,
	                                                             SAMPLE_MAX);
	this->probe_drops = Output::registerProbe<int>("Phy.drops",
	                                               "frame number", true,
	                                               // we need to sum the drops here !
	                                               SAMPLE_SUM);

	return true;
}

bool AttenuationHandler::process(DvbFrame *dvb_frame, double cn_total)
{
	fmt_id_t modcod_id = 0;
	double min_cn;
	Data payload;

	// Consider that the packet is not dropped  (if its dropped, the probe
	// will be updated later), so that the probe emits a 0 value if necessary.
	this->probe_drops->put(0);

	// Get the MODCOD used to send DVB frame
	// (keep the complete header because we carry useful data)
	switch(dvb_frame->getMessageType())
	{
		case MSG_TYPE_BBFRAME:
		{
			// TODO BBFrame *bbframe = dynamic_cast<BBFrame *>(dvb_frame);
			BBFrame *bbframe = (BBFrame *)dvb_frame;
			modcod_id = bbframe->getModcodId();
			payload = bbframe->getPayload();
		}
		break;

		case MSG_TYPE_DVB_BURST:
		{
			// TODO DvbRcsFrame *dvb_rcs_frame = dynamic_cast<DvbRcsFrame *>(dvb_frame);
			DvbRcsFrame *dvb_rcs_frame = (DvbRcsFrame *)dvb_frame;
			modcod_id = dvb_rcs_frame->getModcodId();
			payload = dvb_rcs_frame->getPayload();
		}
		break;

		default:
		{
			// This message, even though it carries C/N information (is attenuated)
			// is not encoded using a MODCOD, and cannot be dropped.
			return true;
		}
	}

	LOG(this->log_channel, LEVEL_INFO,
	    "Receive frame with MODCOD %u, total C/N = %.2f", modcod_id, cn_total);

	// Update minimal condition threshold
	if(!this->minimal_condition_model->updateThreshold(modcod_id, dvb_frame->getMessageType()))
	{
		LOG(this->log_channel, LEVEL_ERROR,
		    "Threshold update failed");
		return false;
	}

	// TODO this would be better to get minimal condition per source terminal
	//      if we are on regenerative satellite or GW
	//      On terminals,  here we receive all BBFrame on the spot,
	//      some may not contain packets for us but we will still count them in stats
	//      We would have to parse frames in order to remove them from
	//      statistics, this is not efficient 
	//      With physcal layer ACM loop, these frame would be mark as corrupted
	min_cn = this->minimal_condition_model->getMinimalCN();
	this->probe_minimal_condition->put(min_cn);
	LOG(this->log_channel, LEVEL_INFO,
	    "Minimal condition value for MODCOD %u: %.2f dB", modcod_id, min_cn);

	// Insert error if required
	if(!this->error_insertion_model->isToBeModifiedPacket(cn_total, min_cn))
	{
		return true;
	}
	LOG(this->log_channel, LEVEL_DEBUG,
	    "Error insertion is required");

	if(!this->error_insertion_model->modifyPacket(payload))
	{
		LOG(this->log_channel, LEVEL_ERROR,
		    "Error insertion failed");
		return false;
	}
	dvb_frame->setCorrupted(true);
	this->probe_drops->put(1);
	LOG(this->log_channel, LEVEL_NOTICE,
	    "Received frame was corrupted");

	return true;
}
