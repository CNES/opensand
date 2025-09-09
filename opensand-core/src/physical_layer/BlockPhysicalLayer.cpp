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


#include <opensand_output/Output.h>
#include <opensand_rt/TimerEvent.h>
#include <opensand_rt/MessageEvent.h>

#include "BlockPhysicalLayer.h"

#include "Plugin.h"
#include "PhysicalLayerPlugin.h"
#include "OpenSandFrames.h"
#include "OpenSandCore.h"
#include "OpenSandPlugin.h"
#include "OpenSandModelConf.h"
#include "NetContainer.h"


BlockPhysicalLayer::BlockPhysicalLayer(const std::string &name, PhyLayerConfig config):
	Rt::Block<BlockPhysicalLayer, PhyLayerConfig>{name, config},
	mac_id(config.mac_id)
{
}


void BlockPhysicalLayer::generateConfiguration()
{
	auto Conf = OpenSandModelConf::Get();
	auto conf = Conf->getOrCreateComponent("physical_layer", "Physical Layer", "The Physical layer configuration");
	auto delay = Conf->getOrCreateComponent("delay", "Delay", conf);
	Plugin::generatePluginsConfiguration(delay, PluginType::SatDelay, "delay_type", "Delay Type");

	AttenuationHandler::generateConfiguration();
	GroundPhysicalChannel::generateConfiguration();
}


bool BlockPhysicalLayer::onInit()
{
	std::string satdelay_name;
	auto delay = OpenSandModelConf::Get()->getProfileData()->getComponent("physical_layer")->getComponent("delay");
	if(!OpenSandModelConf::extractParameterData(delay->getParameter("delay_type"), satdelay_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'physical_layer', missing parameter 'delay_type'");
		return false;
	}

	/// Load de SatDelay Plugin
	this->satdelay = Plugin::getSatDelayPlugin(satdelay_name);
	// Check if the plugin was found
	if(!this->satdelay)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Satellite delay plugin conf was not found for terminal %u",
		    this->mac_id);
		return false;
	}
	// init plugin
	if(!this->satdelay->init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot initialize sat delay plugin '%s' for terminal id %u",
		    satdelay_name.c_str(), this->mac_id);
		return false;
	}

	// share the plugin to channels
	this->upward.setSatDelay(this->satdelay);
	this->downward.setSatDelay(this->satdelay);

	return true;
}


Rt::UpwardChannel<BlockPhysicalLayer>::UpwardChannel(const std::string &name, PhyLayerConfig config):
	GroundPhysicalChannel{config},
	Channels::Upward<UpwardChannel<BlockPhysicalLayer>>{name},
	attenuation_hdl{this->log_channel}
{
}


bool Rt::UpwardChannel<BlockPhysicalLayer>::onInit()
{
	// Initialize parent class
	if(!this->initGround(true, *this, this->log_init))
	{
		return false;
	}

	// generate probes prefix
	bool is_sat = OpenSandModelConf::Get()->getComponentType() == Component::satellite;
	std::string prefix = generateProbePrefix(spot_id, entity_type, is_sat);

	// Initialize the total CN probe
	this->probe_total_cn = Output::Get()->registerProbe<float>(prefix + "Phy.Total_cn", "dB", true, SAMPLE_LAST);

	// Initialize the attenuation handler
	if(!this->attenuation_hdl.initialize(this->log_init, prefix))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Unable to initialize Attenuation Handler");
		return false;
	}

	return true;
}


bool Rt::UpwardChannel<BlockPhysicalLayer>::onEvent(const Event &event)
{
	LOG(this->log_event, LEVEL_ERROR,
	    "Unknown event received %s",
	    event.getName().c_str());
	return false;
}


bool Rt::UpwardChannel<BlockPhysicalLayer>::onEvent(const TimerEvent &event)
{
	if(event == this->fifo_timer)
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
	else if(event == this->attenuation_update_timer)
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

	return true;
}


bool Rt::UpwardChannel<BlockPhysicalLayer>::onEvent(const MessageEvent &event)
{
	LOG(this->log_event, LEVEL_DEBUG, "Incoming DVB frame");
	Ptr<DvbFrame> dvb_frame = event.getMessage<DvbFrame>();

	// Ignore SAC messages if ST
	LOG(this->log_event, LEVEL_DEBUG,
	    "Check the entity is a ST and DVB frame is SAC");
	if(!OpenSandModelConf::Get()->isGw(this->mac_id) &&
	   dvb_frame->getMessageType() == EmulatedMessageType::Sac)
	{
		LOG(this->log_event, LEVEL_DEBUG,
		    "The SAC is deleted because the entity is not a GW");
		return true;
	}

	// Check a delay is applicable to the packet
	LOG(this->log_event, LEVEL_DEBUG,
	    "Check the DVB frame has to be delayed");
	if(IsDelayedFrame(dvb_frame->getMessageType()))
	{
		LOG(this->log_event, LEVEL_DEBUG,
		    "Push the DVB frame in delay FIFO");
		return this->pushPacket(std::move(dvb_frame));
	}

	// Forward packet
	LOG(this->log_event, LEVEL_DEBUG,
	    "Forward the DVB frame");
	if(!this->forwardPacket(std::move(dvb_frame)))
	{
		LOG(this->log_event, LEVEL_ERROR,
		    "DVB frame forwarding failed");
		return false;
	}

	return true;
}


bool Rt::UpwardChannel<BlockPhysicalLayer>::forwardPacket(Ptr<DvbFrame> dvb_frame)
{
	if(IsCnCapableFrame(dvb_frame->getMessageType()))
	{
		// Set C/N to Dvb frame
		auto cn = this->getCn(*dvb_frame);
		dvb_frame->setCn(cn);
		LOG(this->log_event, LEVEL_DEBUG,
		    "Set C/N to the DVB frame forwardPacket %f. Message type %d\n",
		    cn, dvb_frame->getMessageType());

		// Update probe
		this->probe_total_cn->put(dvb_frame->getCn());
	}

	if(IsAttenuatedFrame(dvb_frame->getMessageType()))
	{
		// Process Attenuation
		auto cn = dvb_frame->getCn();
		if(!this->attenuation_hdl.process(*dvb_frame, cn))
		{
			LOG(this->log_event, LEVEL_ERROR,
			    "Failed to get the attenuation");
			return false;
		}
	}

	// Send frame to upper layer
	if (!this->enqueueMessage(std::move(dvb_frame), to_underlying(InternalMessageType::unknown)))
	{
		LOG(this->log_send, LEVEL_ERROR, 
		    "Failed to send burst of packets to upper layer");
		return false;
	}
	return true;
}


double Rt::UpwardChannel<BlockPhysicalLayer>::getCn(DvbFrame &dvb_frame) const
{
	//return GroundPhysicalChannel::computeTotalCn(dvb_frame.getCn(), this->getCurrentCn());
	return this->computeTotalCn(dvb_frame.getCn());
}


Rt::DownwardChannel<BlockPhysicalLayer>::DownwardChannel(const std::string &name, PhyLayerConfig config):
	GroundPhysicalChannel{config},
	Channels::Downward<DownwardChannel<BlockPhysicalLayer>>{name},
	delay_update_timer{-1}
{
}


bool Rt::DownwardChannel<BlockPhysicalLayer>::onInit()
{
	// Initialize parent class
	if(!this->initGround(false, *this, this->log_init))
	{
		return false;
	}

	// generate probes prefix
	bool is_sat = OpenSandModelConf::Get()->getComponentType() == Component::satellite;
	std::string prefix = generateProbePrefix(spot_id, entity_type, is_sat);

	// Initialize the delay event
	this->delay_update_timer = this->addTimerEvent("delay_timer",
	                                               ArgumentWrapper(this->satdelay_model->getRefreshPeriod()));

	// Initialize the delay probe
	this->probe_delay = Output::Get()->registerProbe<int>(prefix + "Phy.Delay", "ms", true, SAMPLE_LAST);

	return true;
}


bool Rt::DownwardChannel<BlockPhysicalLayer>::onEvent(const Event &event)
{
	LOG(this->log_event, LEVEL_ERROR,
	    "unknown event received %s",
	    event.getName().c_str());
	return false;
}


bool Rt::DownwardChannel<BlockPhysicalLayer>::onEvent(const TimerEvent &event)
{
	if(event == this->fifo_timer)
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
	else if(event == this->attenuation_update_timer)
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
	else if(event == this->delay_update_timer)
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

	return true;
}


bool Rt::DownwardChannel<BlockPhysicalLayer>::onEvent(const MessageEvent &event)
{
	LOG(this->log_event, LEVEL_DEBUG,
	    "Incoming DVB frame");
	Ptr<DvbFrame> dvb_frame = event.getMessage<DvbFrame>();

	// Prepare packet
	this->preparePacket(*dvb_frame);

	// Check a delay is applicable to the packet
	LOG(this->log_event, LEVEL_DEBUG,
	    "Check the DVB frame has to be delayed");
	if(IsDelayedFrame(dvb_frame->getMessageType()))
	{
		LOG(this->log_event, LEVEL_DEBUG,
		    "Push the DVB frame in delay FIFO");
		return this->pushPacket(std::move(dvb_frame));
	}

	// Forward packet
	LOG(this->log_event, LEVEL_DEBUG, "Forward the DVB frame");
	if(!this->forwardPacket(std::move(dvb_frame)))
	{
		LOG(this->log_event, LEVEL_ERROR,
		    "The DVB frame forwarding failed");
		return false;
	}

	return true;
}

void Rt::DownwardChannel<BlockPhysicalLayer>::preparePacket(DvbFrame &dvb_frame)
{
	if(IsCnCapableFrame(dvb_frame.getMessageType()))
	{
		// Set C/N to Dvb frame
		LOG(this->log_event, LEVEL_DEBUG,
		    "Set C/N to the DVB frame preparePacket %f. Message type %d\n",
		    this->getCurrentCn(), dvb_frame.getMessageType());
		dvb_frame.setCn(this->getCurrentCn());
	}
}


bool Rt::DownwardChannel<BlockPhysicalLayer>::updateDelay()
{
	LOG(this->log_channel, LEVEL_DEBUG, "Update delay");
	if(!this->satdelay_model->updateSatDelay())
	{
		LOG(this->log_channel, LEVEL_ERROR,
		    "Satellite delay update failed");
		return false;
	}

	time_ms_t delay = this->satdelay_model->getSatDelay();

	LOG(this->log_channel, LEVEL_INFO,
		"New delay: %f ms",
		delay);
	this->probe_delay->put(std::chrono::duration_cast<std::chrono::duration<int, std::milli>>(delay).count());

	return true;
}

bool Rt::DownwardChannel<BlockPhysicalLayer>::forwardPacket(Ptr<DvbFrame> dvb_frame)
{
	// Send frame to upper layer
	if (!this->enqueueMessage(std::move(dvb_frame), to_underlying(InternalMessageType::unknown)))
	{
		LOG(this->log_send, LEVEL_ERROR, 
		    "Failed to send burst of packets to upper layer");
		return false;
	}
	return true;	

}
