/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 CNES
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
 */

// FIXME we need to include uti_debug.h before...
#define DBG_PREFIX
#define DBG_PACKAGE PKG_PHY_LAYER
#include <opensand_conf/uti_debug.h>

#include "BlockPhysicalLayer.h"

#include "Plugin.h"
#include "OpenSandFrames.h"
#include "PhyChannel.h"


#include <opensand_conf/conf.h>


// output events
Event *BlockPhysicalLayer::error_init = NULL;
Event *BlockPhysicalLayer::init_done = NULL;

BlockPhysicalLayer::BlockPhysicalLayer(const string &name):
	Block(name)
{
	error_init = Output::registerEvent("BlockPhysicalLayer::init", LEVEL_ERROR);
	init_done = Output::registerEvent("BlockPhysicalLayer::init_done", LEVEL_INFO);
}


BlockPhysicalLayer::~BlockPhysicalLayer()
{
}


bool BlockPhysicalLayer::onInit(void)
{
	return true;
}

bool BlockPhysicalLayer::onEvent(const RtEvent *const event,
                                 Chan *chan)
{
	switch(event->getType())
	{
		case evt_message:
		{
			// message event: forward DVB frames from upper block to lower block
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			// forward the DVB frame to the lower block
			return chan->forwardFrame(dvb_frame);
		}
		break;

		case evt_timer:
			if(*event != chan->att_timer)
			{
				return false;
			}
			//Event handler for Channel(s) state update
			UTI_DEBUG_L3("channel timer expired\n");
			if(!chan->update())
			{
				UTI_ERROR("one of both channels updating failed, do not "
				          "update channels anymore\n");
				return false;
			}
			break;

		default:
			UTI_ERROR("unknown event received %s",
			          event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockPhysicalLayer::onDownwardEvent(const RtEvent *const event)
{
	return this->onEvent(event, (Chan *)this->downward);
}

bool BlockPhysicalLayer::onUpwardEvent(const RtEvent *const event)
{
	return this->onEvent(event, (Chan *)this->upward);
}

bool BlockPhysicalLayer::PhyUpward::onInit(void)
{
	ostringstream name;
	string link("down"); // we are on downlink

	// Intermediate variables for Config file reading
	string sat_type;
	string attenuation_type;
	string minimal_type;
	string error_type;

	// get granularity
	if(!globalConfig.getValue(PHYSICAL_LAYER_SECTION, GRANULARITY,
	                          this->granularity))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          PHYSICAL_LAYER_SECTION, GRANULARITY);
		goto error;
	}
	UTI_INFO("granularity = %d\n", this->granularity);

	// Initiate Attenuation model
	if(!globalConfig.getValue(DOWNLINK_PHYSICAL_LAYER_SECTION,
	                          ATTENUATION_MODEL_TYPE,
	                          attenuation_type))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DOWNLINK_PHYSICAL_LAYER_SECTION, ATTENUATION_MODEL_TYPE);
		goto error;
	}

	// Initiate Nominal value
	if(!globalConfig.getValue(DOWNLINK_PHYSICAL_LAYER_SECTION,
	                          NOMINAL_CONDITION,
	                          this->nominal_condition))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DOWNLINK_PHYSICAL_LAYER_SECTION, ATTENUATION_MODEL_TYPE);
		goto error;
	}

	// Initiate Minimal conditions
	if(!globalConfig.getValue(DOWNLINK_PHYSICAL_LAYER_SECTION,
	                          MINIMAL_CONDITION_TYPE,
	                          minimal_type))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DOWNLINK_PHYSICAL_LAYER_SECTION, MINIMAL_CONDITION_TYPE);
		goto error;
	}

	// Initiate Error Insertion
	if(!globalConfig.getValue(DOWNLINK_PHYSICAL_LAYER_SECTION,
	                          ERROR_INSERTION_TYPE,
	                          error_type))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
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
		UTI_ERROR("error when getting physical layer plugins");
		goto error;
	}
	if(!this->attenuation_model->init(this->granularity, link))
	{
		UTI_ERROR("cannot initialize attenuation model plugin %s",
		           attenuation_type.c_str());
		goto error;
	}

	UTI_INFO("%slink: attenuation model = %s, nominal condition = %u, "
	         "minimal condition type = %s, error insertion type = %s",
	         link.c_str(), attenuation_type.c_str(), this->nominal_condition,
	         minimal_type.c_str(), error_type.c_str());

	if(!this->minimal_condition->init())
	{
		UTI_ERROR("cannot initialize minimal condition plugin %s",
		          minimal_type.c_str());
		goto error;
	}

	if(!this->error_insertion->init())
	{
		UTI_ERROR("cannot initialize error insertion plugin %s",
		          error_type.c_str());
		goto error;
	}

	name << "attenuation_" << link;
	this->att_timer = this->addTimerEvent(name.str(), this->granularity);

	this->probe_attenuation = Output::registerProbe<float>("dB", true,
	                                                       SAMPLE_MAX,
	                                                       "Phy.%slink_attenuation (%s)",
	                                                       link.c_str(),
	                                                       attenuation_type.c_str());
	this->probe_minimal_condition = Output::registerProbe<float>("dB", true,
	                                                             SAMPLE_MAX,
	                                                             "Phy.minimal_condition (%s)",
	                                                             minimal_type.c_str());
	this->probe_nominal_condition = Output::registerProbe<float>("dB", true,
	                                                             SAMPLE_MAX,
	                                                             "Phy.%slink_nominal_condition",
	                                                             link.c_str());
	// no useful on GW because it depends on terminals and we do not make the difference here
	this->probe_total_cn = Output::registerProbe<float>("dB", true,
	                                                    SAMPLE_MAX,
	                                                    "Phy.%slink_total_cn",
	                                                    link.c_str());
	this->probe_drops = Output::registerProbe<int>("Phy.drops",
	                                               "frame number", true,
	                                               // we need to sum the drops here !
	                                               SAMPLE_SUM);

	return true;

error:
	return false;
}

bool BlockPhysicalLayer::PhyDownward::onInit(void)
{
	ostringstream name;
	string link("up"); // we are on uplink

	// Intermediate variables for Config file reading
	string attenuation_type;


	// get granularity
	if(!globalConfig.getValue(PHYSICAL_LAYER_SECTION, GRANULARITY,
	                          this->granularity))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          PHYSICAL_LAYER_SECTION, GRANULARITY);
		goto error;
	}
	UTI_INFO("granularity = %d\n", this->granularity);

	// Initiate Attenuation model
	if(!globalConfig.getValue(UPLINK_PHYSICAL_LAYER_SECTION,
	                          ATTENUATION_MODEL_TYPE,
	                          attenuation_type))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          UPLINK_PHYSICAL_LAYER_SECTION, ATTENUATION_MODEL_TYPE);
		goto error;
	}

	// Initiate Nominal value
	if(!globalConfig.getValue(UPLINK_PHYSICAL_LAYER_SECTION,
	                          NOMINAL_CONDITION,
	                          this->nominal_condition))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          UPLINK_PHYSICAL_LAYER_SECTION, ATTENUATION_MODEL_TYPE);
		goto error;
	}

	/* get all the plugins */
	if(!Plugin::getPhysicalLayerPlugins(attenuation_type,
	                                    "", "",
	                                    &this->attenuation_model,
	                                    NULL, NULL))
	{
		UTI_ERROR("error when getting physical layer plugins");
		goto error;
	}
	if(!this->attenuation_model->init(this->granularity, link))
	{
		UTI_ERROR("cannot initialize attenuation model plugin %s",
		           attenuation_type.c_str());
		goto error;
	}

	UTI_INFO("%slink: attenuation model = %s, nominal condition = %u",
	         link.c_str(), attenuation_type.c_str(), this->nominal_condition);

	name << "attenuation_" << link;
	this->att_timer = this->addTimerEvent(name.str(), this->granularity);

	this->probe_attenuation = Output::registerProbe<float>("dB", true,
	                                                       SAMPLE_LAST,
	                                                       "Phy.%slink_attenuation (%s)",
	                                                       link.c_str(),
	                                                       attenuation_type.c_str());
	this->probe_nominal_condition = Output::registerProbe<float>("dB", true,
	                                                             SAMPLE_MAX,
	                                                             "Phy.%slink_nominal_condition",
	                                                             link.c_str());

	return true;

error:
	return false;

}


bool BlockPhysicalLayer::PhyUpward::forwardFrame(DvbFrame *dvb_frame)
{
	double cn_total;

	if(!IS_DATA_FRAME(dvb_frame->getMessageType()))
	{
		// do not handle signalisation
		goto forward;
	}

	// Update of the Threshold CN if Minimal Condition
	// Mde is Modcod dependent
	if(!this->updateMinimalCondition(dvb_frame))
	{
		// debug because it will be very verbose
		UTI_DEBUG("Error in Update of Minimal Condition\n");
		goto error;
	}

	UTI_DEBUG_L3("Received DVB frame on carrier %u: C/N  = %.2f\n",
	             dvb_frame->getCarrierId(),
	             dvb_frame->getCn());

	cn_total = this->getTotalCN(dvb_frame);
	UTI_DEBUG("Total C/N: %.2f dB\n", cn_total);
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
		UTI_ERROR("failed to send burst of packets to upper layer\n");
		goto error;
	}

	return true;
error:
	delete dvb_frame;
	return false;
}



bool BlockPhysicalLayer::PhyDownward::forwardFrame(DvbFrame *dvb_frame)
{
	if(!IS_DATA_FRAME(dvb_frame->getMessageType()))
	{
		// do not handle signalisation
		goto forward;
	}

	// Case of outgoing msg: Mark the msg with the C/N of Channel
	this->addSegmentCN(dvb_frame);

	UTI_DEBUG_L3("Send DVB frame on carrier %u: C/N  = %.2f\n",
	             dvb_frame->getCarrierId(),
	             dvb_frame->getCn());

forward:
	// message successfully created, send the message to lower block
	if(!this->enqueueMessage((void **)&dvb_frame))
	{
		UTI_ERROR("failed to send burst of packets to lower layer\n");
		goto error;
	}

	return true;
error:
	delete dvb_frame;
	return false;
}


