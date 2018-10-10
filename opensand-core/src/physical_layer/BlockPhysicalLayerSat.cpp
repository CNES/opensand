/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 CNES
 * Copyright © 2017 TAS
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
 * @file     BlockPhysicalLayerSat.cpp
 * @brief    PhysicalLayer block for regenerative satellite
 * @author   Santiago PENA <santiago.penaluque@cnes.fr>
 * @author   Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author   Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 */

#include "BlockPhysicalLayerSat.h"

#include "Plugin.h"
#include "OpenSandFrames.h"
#include "OpenSandCore.h"
#include "OpenSandConf.h"

#include <opensand_output/Output.h>
#include <opensand_conf/conf.h>

BlockPhysicalLayerSat::Upward::Upward(const string &name):
	RtUpward(name),
	log_channel(NULL),
	log_event(NULL),
	attenuation_hdl(NULL)
{
	this->log_channel = Output::registerLog(LEVEL_WARNING, "PhysicalLayer.Channel");
	this->log_event = Output::registerLog(LEVEL_WARNING, "PhysicalLayer.Upward.Event");
}

BlockPhysicalLayerSat::Upward::~Upward()
{
	if(this->attenuation_hdl)
	{
		delete this->attenuation_hdl;
		this->attenuation_hdl = NULL;
	}
}

bool BlockPhysicalLayerSat::Upward::forwardPacket(DvbFrame *dvb_frame)
{
	// Forward packet
	if(this->attenuation_hdl)
	{
		if(!this->forwardPacketWithAttenuation(dvb_frame))
		{
			return false;
		}
	}
	else
	{
		if(!this->forwardPacketWithoutAttenuation(dvb_frame))
		{
			return false;
		}
	}
	return true;
}

bool BlockPhysicalLayerSat::Upward::forwardPacketWithoutAttenuation(DvbFrame *dvb_frame)
{
	// Send frame to upper layer
	if(!this->enqueueMessage((void **)&dvb_frame))
	{
		LOG(this->log_send, LEVEL_ERROR, 
		    "Failed to send burst of packets to upper layer");
		delete dvb_frame;
		return false;
	}
	return true;
}

bool BlockPhysicalLayerSat::Upward::forwardPacketWithAttenuation(DvbFrame *dvb_frame)
{
	if(IS_ATTENUATED_FRAME(dvb_frame->getMessageType()))
	{
		// Process Attenuation
		if(!this->attenuation_hdl->process(dvb_frame, dvb_frame->getCn()))
		{
			LOG(this->log_event, LEVEL_ERROR,
			    "Failed to get the attenuation");
			delete dvb_frame;
			return false;
		}
	}

	// Send frame to upper layer
	if(!this->enqueueMessage((void **)&dvb_frame))
	{
		LOG(this->log_send, LEVEL_ERROR, 
		    "Failed to send burst of packets to upper layer");
		delete dvb_frame;
		return false;
	}
	return true;
}

bool BlockPhysicalLayerSat::Upward::onInit()
{
	bool attenuation_enabled = true;

	// Check attenuation activation
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION],
	                   ENABLE, attenuation_enabled))
	{
		LOG(log_init, LEVEL_ERROR,
		    "Unable to check if physical layer is enabled");
		return false;
	}
	if(!attenuation_enabled)
	{
		return true;
	}

	// Initialize the attenuation handler
	this->attenuation_hdl = new AttenuationHandler(this->log_channel);
  if(!this->attenuation_hdl->initialize(UPLINK_PHYSICAL_LAYER_SECTION,
                                        this->log_init))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "Unable to initialize Attenuation Handler");
    return false;
  }

	//TODO: set the function pointer to 'forwardPacketWithAttenuation'
	return true;
}

bool BlockPhysicalLayerSat::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			// Forward packet
			if(!this->forwardPacket(dvb_frame))
			{
				return false;
			}

			// Send frame to upper layer
			if(!this->enqueueMessage((void **)&dvb_frame))
			{
				LOG(this->log_send, LEVEL_ERROR, 
				    "Failed to send burst of packets to upper layer");
				delete dvb_frame;
				return false;
			}
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

BlockPhysicalLayerSat::Downward::Downward(const string &name):
	RtDownward(name),
	log_event(NULL)
{
	this->log_event = Output::registerLog(LEVEL_WARNING, "PhysicalLayer.Downward.Event");
}

bool BlockPhysicalLayerSat::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			// Send frame to upper layer
			if(!this->enqueueMessage((void **)&dvb_frame))
			{
				LOG(this->log_send, LEVEL_ERROR, 
				    "Failed to send burst of packets to upper layer");
				delete dvb_frame;
				return false;
			}
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
