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
 * @file     BlockPhysicalLayer.cpp
 * @brief    PhysicalLayer block
 * @author   Santiago PENA <santiago.penaluque@cnes.fr>
 * @author   Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author   Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 */

#include "BlockPhysicalLayer.h"

#include "Plugin.h"
#include "OpenSandFrames.h"
#include "OpenSandCore.h"
#include "OpenSandPlugin.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>

BlockPhysicalLayer::BlockPhysicalLayer(const string &name, tal_id_t mac_id):
	Block(name),
	mac_id(mac_id),
	satdelay(NULL)
{
}


BlockPhysicalLayer::~BlockPhysicalLayer()
{
}


void BlockPhysicalLayer::generateConfiguration()
{
	auto Conf = OpenSandModelConf::Get();
	auto conf = Conf->getOrCreateComponent("physical_layer", "Physical Layer", "The Physical layer configuration");
	auto delay = Conf->getOrCreateComponent("delay", "Delay", conf);
	Plugin::generatePluginsConfiguration(delay, satdelay_plugin, "delay_type", "Delay Type");

	AttenuationHandler::generateConfiguration();
	GroundPhysicalChannel::generateConfiguration();
}


bool BlockPhysicalLayer::onInit(void)
{
	uint8_t id;

	std::string satdelay_name;
	auto delay = OpenSandModelConf::Get()->getProfileData()->getComponent("physical_layer")->getComponent("delay");
	if(!OpenSandModelConf::extractParameterData(delay->getParameter("delay_type"), satdelay_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'physical_layer', missing parameter 'delay_type'");
		return false;
	}

	/// Load de SatDelay Plugin
	if(!Plugin::getSatDelayPlugin(satdelay_name, &this->satdelay))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "error when getting the sat delay plugin '%s'",
		    satdelay_name.c_str());
		return false;
	}
	// Check if the plugin was found
	if(this->satdelay == NULL)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Satellite delay plugin conf was not found for"
		    " terminal %s", this->mac_id);
		return false;
	}
	// init plugin
	if(!this->satdelay->init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot initialize sat delay plugin '%s'"
		    " for terminal id %u ", satdelay_name.c_str(), id);
		return false;
	}

	// share the plugin to channels
	((Upward *)this->upward)->setSatDelay(this->satdelay);
	((Downward *)this->downward)->setSatDelay(this->satdelay);

	return true;
}

BlockPhysicalLayer::Upward::Upward(const string &name, tal_id_t mac_id):
	GroundPhysicalChannel(mac_id),
	RtUpward(name),
	probe_total_cn(NULL),
	attenuation_hdl(NULL)
{
}

BlockPhysicalLayer::Upward::~Upward()
{
	if(this->attenuation_hdl)
	{
		delete this->attenuation_hdl;
		this->attenuation_hdl = NULL;
	}
}

bool BlockPhysicalLayer::Upward::onInit()
{
	// Initialize parent class
	if(!this->initGround(true, this, this->log_init))
	{
		return false;
	}

	// Initialize the total CN probe
	this->probe_total_cn = Output::Get()->registerProbe<float>("Phy.Total_cn", "dB", true, SAMPLE_LAST);

	// Initialize the attenuation handler
	this->attenuation_hdl = new AttenuationHandler(this->log_channel);
	if(!this->attenuation_hdl->initialize(this->log_init))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Unable to initialize Attenuation Handler");
		return false;
	}

	return true;
}

bool BlockPhysicalLayer::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			LOG(this->log_event, LEVEL_DEBUG,
			    "Incoming DVB frame");
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			// Ignore SAC messages if ST
			LOG(this->log_event, LEVEL_DEBUG,
			    "Check the entity is a ST and DVB frame is SAC");
			if(!OpenSandModelConf::Get()->isGw(this->mac_id) &&
			   dvb_frame->getMessageType() == MSG_TYPE_SAC)
			{
				LOG(this->log_event, LEVEL_DEBUG,
				    "The SAC is deleted because the entity is not a GW");
				delete dvb_frame;
				return true;
			}

			// Check a delay is applicable to the packet
			LOG(this->log_event, LEVEL_DEBUG,
			    "Check the DVB frame has to be delayed");
			if(IS_DELAYED_FRAME(dvb_frame->getMessageType()))
			{
				LOG(this->log_event, LEVEL_DEBUG,
				    "Push the DVB frame in delay FIFO");
				return this->pushPacket((NetContainer *)dvb_frame);
			}

			// Forward packet
			LOG(this->log_event, LEVEL_DEBUG,
			    "Forward the DVB frame");
			if(!this->forwardPacket(dvb_frame))
			{
				LOG(this->log_event, LEVEL_ERROR,
				    "DVB frame forwarding failed");
				return false;
			}
		}
		break;

		case evt_timer:
		{
			if(*event == this->fifo_timer)
			{
				// Event handler for delay FIFO
				LOG(this->log_event, LEVEL_DEBUG,
				    "Delay FIFO timer expired");
				if(!this->forwardReadyPackets())
				{
					LOG(this->log_event, LEVEL_ERROR,
					    "Delayed packets forwarding failed");
					return false;
				}

			}
			else if(*event == this->attenuation_update_timer)
			{
				// Event handler for Upward Channel state update
				LOG(this->log_event, LEVEL_DEBUG,
				    "Attenuation update timer expired");
				if(!this->updateAttenuation())
				{
					LOG(this->log_event, LEVEL_ERROR,
					    "Attenuation update failed");
					return false;
				}
			}
			else
			{
				LOG(this->log_event, LEVEL_ERROR,
				    "Unknown timer event received");
				return false;
			}
		}
		break;
			
		default:
			LOG(this->log_event, LEVEL_ERROR,
			    "Unknown event received %s",
			    event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockPhysicalLayer::Upward::forwardPacket(DvbFrame *dvb_frame)
{
	if(IS_CN_CAPABLE_FRAME(dvb_frame->getMessageType()))
	{
		// Set C/N to Dvb frame
		dvb_frame->setCn(this->getCn(dvb_frame));
		LOG(this->log_event, LEVEL_DEBUG,
		    "Set C/N to the DVB frame forwardPacket %f. Message type %d\n", this->getCn(dvb_frame), dvb_frame->getMessageType());

		// Update probe
		this->probe_total_cn->put(dvb_frame->getCn());
	}

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

double BlockPhysicalLayer::Upward::getCn(DvbFrame *dvb_frame) const
{
	return GroundPhysicalChannel::computeTotalCn(dvb_frame->getCn(), this->getCurrentCn());
}

BlockPhysicalLayer::Downward::Downward(const string &name, tal_id_t mac_id):
	GroundPhysicalChannel(mac_id),
	RtDownward(name),
	probe_delay(NULL),
	delay_update_timer(-1)
{
}

bool BlockPhysicalLayer::Downward::onInit()
{
	// Initialize parent class
	if(!this->initGround(false, this, this->log_init))
	{
		return false;
	}

	// Initialize the delay event
	this->delay_update_timer = this->addTimerEvent("delay_timer",
                                                       this->satdelay_model->getRefreshPeriod());

	// Initialize the delay probe
	this->probe_delay = Output::Get()->registerProbe<int>("Phy.Delay", "ms", true, SAMPLE_LAST);

	return true;
}

bool BlockPhysicalLayer::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			LOG(this->log_event, LEVEL_DEBUG,
			    "Incoming DVB frame");
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			// Prepare packet
			this->preparePacket(dvb_frame);

			// Check a delay is applicable to the packet
			LOG(this->log_event, LEVEL_DEBUG,
			    "Check the DVB frame has to be delayed");
			if(IS_DELAYED_FRAME(dvb_frame->getMessageType()))
			{
				LOG(this->log_event, LEVEL_DEBUG,
				    "Push the DVB frame in delay FIFO");
				return this->pushPacket((NetContainer *)dvb_frame);
			}

			// Forward packet
			LOG(this->log_event, LEVEL_DEBUG,
			    "Forward the DVB frame");
			if(!this->forwardPacket(dvb_frame))
			{
				LOG(this->log_event, LEVEL_ERROR,
				    "The DVB frame forwarding failed");
				return false;
			}
		}
		break;

		case evt_timer:
		{
			if(*event == this->fifo_timer)
			{
				// Event handler for delay FIFO
				LOG(this->log_event, LEVEL_DEBUG,
				    "Delay FIFO timer expired");
				if(!this->forwardReadyPackets())
				{
					LOG(this->log_event, LEVEL_ERROR,
					    "Delayed packets forwarding failed");
					return false;
				}
			}
			else if(*event == this->attenuation_update_timer)
			{
				// Event handler for Upward Channel state update
				LOG(this->log_event, LEVEL_DEBUG,
				    "Attenuation update timer expired");
				if(!this->updateAttenuation())
				{
					LOG(this->log_event, LEVEL_ERROR,
					    "Attenuation update failed");
					return false;
				}
			}
			else if(*event == this->delay_update_timer)
			{
				// Event handler for update satellite delay
				LOG(this->log_event, LEVEL_DEBUG,
				    "Delay update timer expired");
				if(!this->updateDelay())
				{
					LOG(this->log_event, LEVEL_ERROR,
					    "Satellite delay update failed");
					return false;
				}
				// Send probes
				Output::Get()->sendProbes();
			}
			else
			{
				LOG(this->log_event, LEVEL_ERROR,
				    "unknown timer event received");
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

void BlockPhysicalLayer::Downward::preparePacket(DvbFrame *dvb_frame)
{
	if(IS_CN_CAPABLE_FRAME(dvb_frame->getMessageType()))
	{
		// Set C/N to Dvb frame
		LOG(this->log_event, LEVEL_DEBUG,
		    "Set C/N to the DVB frame preparePacket %f. Message type %d\n", this->getCurrentCn(), dvb_frame->getMessageType());
		dvb_frame->setCn(this->getCurrentCn());
	}
}

bool BlockPhysicalLayer::Downward::updateDelay()
{
	LOG(this->log_channel, LEVEL_DEBUG,
		"Update delay");
	if(!this->satdelay_model->updateSatDelay())
	{
		LOG(this->log_channel, LEVEL_ERROR,
		    "Satellite delay update failed");
		return false;
	}

	time_ms_t delay = this->satdelay_model->getSatDelay();

	LOG(this->log_channel, LEVEL_INFO,
		"New delay: %u ms",
		delay);
	this->probe_delay->put(delay);

	return true;
}

bool BlockPhysicalLayer::Downward::forwardPacket(DvbFrame *dvb_frame)
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
