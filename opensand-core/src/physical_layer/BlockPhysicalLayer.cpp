/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 CNES
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
 * @file     BlockPhysicalLayer.cpp
 * @brief    PhysicalLayer bloc
 * @author   Santiago PENA <santiago.penaluque@cnes.fr>
 * @author   Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include "BlockPhysicalLayer.h"

#include "Plugin.h"
#include "OpenSandFrames.h"
#include "PhyChannel.h"

#include <opensand_output/Output.h>
#include <opensand_conf/conf.h>


BlockPhysicalLayer::BlockPhysicalLayer(const string &name):
	Block(name)
{
}


BlockPhysicalLayer::~BlockPhysicalLayer()
{
}


bool BlockPhysicalLayer::onInit(void)
{
	return true;
}

BlockPhysicalLayer::Downward::Downward(const string &name):
	RtDownward(name),
	PhyChannel(),
	log_event(NULL)
{
	// Output Log
	this->log_event = Output::registerLog(LEVEL_WARNING, "PhysicalLayer.Downward.Event");
}

bool BlockPhysicalLayer::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			// message event: forward DVB frames from upper block to lower block
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			// forward the DVB frame to the lower block
			return this->forwardFrame(dvb_frame);
		}
		break;

		case evt_timer:
			if(*event != this->att_timer)
			{
				return false;
			}
			//Event handler for Downward Channel state update
			LOG(this->log_event, LEVEL_DEBUG,
			    "downward channel timer expired\n");
			if(!this->update())
			{
				LOG(this->log_event, LEVEL_ERROR,
				    "downward channel updating failed, do not "
				    "update channels anymore\n");
				return false;
			}
			break;

		default:
			LOG(this->log_event, LEVEL_ERROR,
			    "unknown event received %s",
			    event->getName().c_str());
			return false;
	}

	return true;
}

BlockPhysicalLayer::Upward::Upward(const string &name):
	RtUpward(name),
	PhyChannel(),
	log_event(NULL)
{
	// Output Log
	this->log_event = Output::registerLog(LEVEL_WARNING, "PhysicalLayer.Upward.Event");
}

bool BlockPhysicalLayer::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			// message event: forward DVB frames from upper block to lower block
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			// forward the DVB frame to the lower block
			return this->forwardFrame(dvb_frame);
		}
		break;

		case evt_timer:
			if(*event != this->att_timer)
			{
				return false;
			}
			//Event handler for Upward Channel state update
			LOG(this->log_event, LEVEL_DEBUG,
			    "upward channel timer expired\n");
			if(!this->update())
			{
				LOG(this->log_event, LEVEL_ERROR,
				    "upward channel updating failed, do not "
				    "update channels anymore\n");
				return false;
			}
			break;

		default:
			LOG(this->log_event, LEVEL_ERROR,
			    "unknown event received %s",
			    event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockPhysicalLayer::Upward::onInit(void)
{
	ostringstream name;
	string link("down"); // we are on downlink

	// Intermediate variables for Config file reading
	string attenuation_type;
	string minimal_type;
	string error_type;

	char probe_name[128];

	// get refresh period
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION], 
		               ACM_PERIOD_REFRESH,
	                   this->refresh_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    PHYSICAL_LAYER_SECTION, ACM_PERIOD_REFRESH);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "acm refreshing period = %d\n", this->refresh_period_ms);

	// Initiate Attenuation model
	if(!Conf::getValue(Conf::section_map[DOWNLINK_PHYSICAL_LAYER_SECTION],
	                   ATTENUATION_MODEL_TYPE,
	                   attenuation_type))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DOWNLINK_PHYSICAL_LAYER_SECTION,
		    ATTENUATION_MODEL_TYPE);
		goto error;
	}

	// Initiate Clear Sky value
	if(!Conf::getValue(Conf::section_map[DOWNLINK_PHYSICAL_LAYER_SECTION],
	                   CLEAR_SKY_CONDITION,
	                   this->clear_sky_condition))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DOWNLINK_PHYSICAL_LAYER_SECTION,
		    ATTENUATION_MODEL_TYPE);
		goto error;
	}

	// Initiate Minimal conditions
	if(!Conf::getValue(Conf::section_map[DOWNLINK_PHYSICAL_LAYER_SECTION],
	                   MINIMAL_CONDITION_TYPE,
	                   minimal_type))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DOWNLINK_PHYSICAL_LAYER_SECTION,
		    MINIMAL_CONDITION_TYPE);
		goto error;
	}

	// Initiate Error Insertion
	if(!Conf::getValue(Conf::section_map[DOWNLINK_PHYSICAL_LAYER_SECTION],
	                   ERROR_INSERTION_TYPE,
	                   error_type))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DOWNLINK_PHYSICAL_LAYER_SECTION, ERROR_INSERTION_TYPE);
		goto error;
	}

	/* get all the plugins */
	if(!Plugin::getPhysicalLayerPlugins(attenuation_type,
	                                    minimal_type,
	                                    error_type,
	                                    &this->attenuation_model,
	                                    &this->minimal_condition,
	                                    &this->error_insertion))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "error when getting physical layer plugins");
		goto error;
	}
	if(!this->attenuation_model->init(this->refresh_period_ms, link))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot initialize attenuation model plugin %s",
		    attenuation_type.c_str());
		goto error;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "%slink: attenuation model = %s, clear_sky condition = %u, "
	    "minimal condition type = %s, error insertion type = %s",
	    link.c_str(), attenuation_type.c_str(),
	    this->clear_sky_condition,
	    minimal_type.c_str(), error_type.c_str());

	if(!this->minimal_condition->init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot initialize minimal condition plugin %s",
		    minimal_type.c_str());
		goto error;
	}

	if(!this->error_insertion->init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot initialize error insertion plugin %s",
		    error_type.c_str());
		goto error;
	}

	name << "attenuation_" << link;
	this->att_timer = this->addTimerEvent(name.str(), this->refresh_period_ms);

	snprintf(probe_name, sizeof(probe_name), 
	         "Phy.%slink_attenuation (%s)",
	         link.c_str(), attenuation_type.c_str());
	this->probe_attenuation = Output::registerProbe<float>(probe_name,
			                                                   "dB", true,
	                                                       SAMPLE_MAX);
	snprintf(probe_name, sizeof(probe_name), 
	         "Phy.minimal_condition (%s)", minimal_type.c_str());
	this->probe_minimal_condition = Output::registerProbe<float>(probe_name,
			                                                         "dB", true,
	                                                             SAMPLE_MAX);
	snprintf(probe_name, sizeof(probe_name), 
	         "Phy.%slink_clear_sky_condition", link.c_str());
	this->probe_clear_sky_condition = Output::registerProbe<float>(probe_name,
	                                                               "dB", true,
	                                                                SAMPLE_MAX);
	// no useful on GW because it depends on terminals and we do not make the difference here
	snprintf(probe_name, sizeof(probe_name), 
	         "Phy.%slink_total_cn", link.c_str());
	this->probe_total_cn = Output::registerProbe<float>(probe_name, "dB", true,
	                                                    SAMPLE_MAX);
	this->probe_drops = Output::registerProbe<int>("Phy.drops",
	                                               "frame number", true,
	                                               // we need to sum the drops here !
	                                               SAMPLE_SUM);

	return true;

error:
	return false;
}

bool BlockPhysicalLayer::Downward::onInit(void)
{
	ostringstream name;
	string link("up"); // we are on uplink
	string attenuation_type;
	char probe_name[128];

	// get refresh_period
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION], 
		               ACM_PERIOD_REFRESH,
	                   this->refresh_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    PHYSICAL_LAYER_SECTION, ACM_PERIOD_REFRESH);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "refresh_period_ms = %d\n", this->refresh_period_ms);

	// Initiate Attenuation model
	if(!Conf::getValue(Conf::section_map[UPLINK_PHYSICAL_LAYER_SECTION],
	                   ATTENUATION_MODEL_TYPE,
	                   attenuation_type))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    UPLINK_PHYSICAL_LAYER_SECTION, ATTENUATION_MODEL_TYPE);
		goto error;
	}

	// Initiate Clear Sky value
	if(!Conf::getValue(Conf::section_map[UPLINK_PHYSICAL_LAYER_SECTION],
	                   CLEAR_SKY_CONDITION,
	                   this->clear_sky_condition))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    UPLINK_PHYSICAL_LAYER_SECTION, ATTENUATION_MODEL_TYPE);
		goto error;
	}

	/* get all the plugins */
	if(!Plugin::getPhysicalLayerPlugins(attenuation_type,
	                                    "", "",
	                                    &this->attenuation_model,
	                                    NULL, NULL))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "error when getting physical layer plugins");
		goto error;
	}
	if(!this->attenuation_model->init(this->refresh_period_ms, link))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot initialize attenuation model plugin %s",
		    attenuation_type.c_str());
		goto error;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "%slink: attenuation model = %s, clear_sky condition = %u",
	    link.c_str(), attenuation_type.c_str(),
	    this->clear_sky_condition);

	name << "attenuation_" << link;
	this->att_timer = this->addTimerEvent(name.str(), this->refresh_period_ms);

	snprintf(probe_name, sizeof(probe_name),
	         "Phy.%slink_attenuation (%s)",
	         link.c_str(), attenuation_type.c_str());
	this->probe_attenuation = Output::registerProbe<float>(probe_name,
	                                                      "dB", true,
	                                                       SAMPLE_LAST);
	snprintf(probe_name, sizeof(probe_name),
	         "Phy.%slink_clear_sky_condition", link.c_str());
	this->probe_clear_sky_condition = Output::registerProbe<float>(probe_name,
	                                                               "dB", true,
	                                                               SAMPLE_MAX);

	return true;

error:
	return false;

}


bool BlockPhysicalLayer::Upward::forwardFrame(DvbFrame *dvb_frame)
{
	double cn_total;

	if(!IS_DATA_FRAME(dvb_frame->getMessageType()))
	{
		// do not handle signalisation but forward it
		goto forward;
	}

	// Update of the Threshold CN if Minimal Condition
	// Mde is Modcod dependent
	if(!this->updateMinimalCondition(dvb_frame))
	{
		// debug because it will be very verbose
		LOG(this->log_send, LEVEL_INFO,
		    "Error in Update of Minimal Condition\n");
		goto error;
	}

	LOG(this->log_send, LEVEL_DEBUG,
	    "Received DVB frame on carrier %u: C/N = %.2f\n",
	    dvb_frame->getCarrierId(),
	    dvb_frame->getCn());

	if(this->is_sat)
	{
		cn_total = dvb_frame->getCn();
		this->probe_total_cn->put(cn_total);
	}
	else
	{
		cn_total = this->getTotalCN(dvb_frame);
	}
	LOG(this->log_send, LEVEL_INFO,
	    "Total C/N: %.2f dB\n", cn_total);
	// Checking if the received frame must be affected by errors
	if(this->isToBeModifiedPacket(cn_total))
	{
		// Insertion of errors if necessary
		this->modifyPacket(dvb_frame);
	}

forward:
	// message successfully created, send the message to upper block
	// transmit the physical parameters as they will be used by DVB layer
	if(!this->enqueueMessage((void **)&dvb_frame))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "failed to send burst of packets to upper layer\n");
		goto error;
	}

	return true;
error:
	delete dvb_frame;
	return false;
}



bool BlockPhysicalLayer::Downward::forwardFrame(DvbFrame *dvb_frame)
{
	if(!IS_DATA_FRAME(dvb_frame->getMessageType()))
	{
		// do not handle signalisation
		goto forward;
	}

	// Case of outgoing msg: Mark the msg with the C/N of Channel
	if(this->is_sat)
	{
		// we only need physical parameters for factorization on the receving side
		// that is the same for transparent mode
		// we use a very high value for unused C/N as it won't change anything
		dvb_frame->setCn(0x0fff);
	}
	else
	{
		this->addSegmentCN(dvb_frame);
	}

	LOG(this->log_send, LEVEL_DEBUG, 
	    "Send DVB frame on carrier %u: C/N  = %.2f\n",
	    dvb_frame->getCarrierId(),
	    dvb_frame->getCn());

forward:
	// message successfully created, send the message to lower block
	if(!this->enqueueMessage((void **)&dvb_frame))
	{
		LOG(this->log_send, LEVEL_ERROR, 
		    "failed to send burst of packets to lower layer\n");
		goto error;
	}

	return true;
error:
	delete dvb_frame;
	return false;
}


bool BlockPhysicalLayerSat::Upward::onInit(void)
{
	ostringstream name;

	string minimal_type;
	string error_type;
	char probe_name[128];

	this->is_sat = true;

	// Initiate Minimal conditions
	if(!Conf::getValue(Conf::section_map[SAT_PHYSICAL_LAYER_SECTION],
	                   MINIMAL_CONDITION_TYPE,
	                   minimal_type))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SAT_PHYSICAL_LAYER_SECTION, MINIMAL_CONDITION_TYPE);
		return false;
	}

	// Initiate Error Insertion
	if(!Conf::getValue(Conf::section_map[SAT_PHYSICAL_LAYER_SECTION],
	            ERROR_INSERTION_TYPE,
	            error_type))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SAT_PHYSICAL_LAYER_SECTION, ERROR_INSERTION_TYPE);
		return false;
	}

	/* get all the plugins */
	if(!Plugin::getPhysicalLayerPlugins("",
	                                    minimal_type,
	                                    error_type,
	                                    &this->attenuation_model,
	                                    &this->minimal_condition,
	                                    &this->error_insertion))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "error when getting physical layer plugins");
		return false;
	}
	
	LOG(this->log_init, LEVEL_NOTICE,
	    "uplink: minimal condition type = %s, error insertion "
	    "type = %s", minimal_type.c_str(), error_type.c_str());

	if(!this->minimal_condition->init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot initialize minimal condition plugin %s",
		    minimal_type.c_str());
		return false;
	}

	if(!this->error_insertion->init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot initialize error insertion plugin %s",
		    error_type.c_str());
		return false;
	}

	snprintf(probe_name, sizeof(probe_name),
	         "Phy.minimal_condition (%s)", minimal_type.c_str());
	this->probe_minimal_condition = Output::registerProbe<float>(probe_name, "dB", true,
	                                                             SAMPLE_MAX);
	// TODO these probes are not really relevant as we should get probes per source
	//      terminal
	this->probe_drops = Output::registerProbe<int>("Phy.drops",
	                                               "frame number", true,
	                                               // we need to sum the drops here !
	                                               SAMPLE_SUM);
	this->probe_total_cn = Output::registerProbe<float>("Phy.uplink_total_cn",
			                                                "dB", true,
	                                                    SAMPLE_MAX);

	return true;

}


bool BlockPhysicalLayerSat::Downward::onInit(void)
{
	this->is_sat = true;
	return true;
}


